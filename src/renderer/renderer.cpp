#include "renderer.hpp"

#include <vuk/src/RenderGraphUtil.hpp>
#include <vuk/Partials.hpp>
#include <backends/imgui_impl_glfw.h>
#include <tracy/Tracy.hpp>
#include <stb_image.h>

#include "extension/fmt_geometry.hpp"
#include "extension/imgui_extra.hpp"
#include "general/file.hpp"
#include "general/hash.hpp"
#include "general/math.hpp"
#include "general/logger.hpp"
#include "game/game.hpp"
#include "game/input.hpp"
#include "editor/console.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/samplers.hpp"
#include "renderer/utils.hpp"
#include "renderer/assets/texture_asset.hpp"
#include "renderer/assets/mesh_asset.hpp"

namespace spellbook {

Renderer::Renderer()
    : imgui_data() {
    vkb::InstanceBuilder builder;
    builder
#if VALIDATION
        .request_validation_layers()
        .set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT                           messageType,
            const VkDebugUtilsMessengerCallbackDataEXT*               pCallbackData,
            void*                                                     pUserData) -> VkBool32 {
                auto ms = vkb::to_string_message_severity(messageSeverity);
                auto mt = vkb::to_string_message_type(messageType);
                console({.str = fmt_("[{}: {}]\n{}\n", ms, mt, pCallbackData->pMessage), .group = "vulkan", .color = palette::crimson});
                __debugbreak();
                return VK_FALSE;
            })
#endif
        .set_app_name("star_stealer")
        .set_engine_name("spellbook")
        .require_api_version(1, 2, 0)
        .set_app_version(0, 1, 0);
    auto inst_ret = builder.build();
    assert_else(inst_ret.has_value())
        return;
    vkbinstance                          = inst_ret.value();
    auto                        instance = vkbinstance.instance;
    vkb::PhysicalDeviceSelector selector{vkbinstance};
    VkPhysicalDeviceFeatures    vkfeatures{
        .independentBlend = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };
    window  = create_window_glfw("Spellbook", window_size, true);
    surface = create_surface_glfw(vkbinstance.instance, window);

    GLFWimage image;
    image.pixels = stbi_load("icon.png", &image.width, &image.height, nullptr, 4);
    glfwSetWindowIcon(window, 1, &image);
    stbi_image_free(image.pixels);

    selector.set_surface(surface)
            .set_minimum_version(1, 0)
            .set_required_features(vkfeatures)
            .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    auto phys_ret = selector.select();
    assert_else(phys_ret.has_value());
    vkb::PhysicalDevice vkbphysical_device = phys_ret.value();
    physical_device                        = vkbphysical_device.physical_device;

    vkb::DeviceBuilder               device_builder{vkbphysical_device};
    VkPhysicalDeviceVulkan12Features vk12features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    vk12features.timelineSemaphore                         = true;
    vk12features.descriptorBindingPartiallyBound           = true;
    vk12features.descriptorBindingUpdateUnusedWhilePending = true;
    vk12features.shaderSampledImageArrayNonUniformIndexing = true;
    vk12features.runtimeDescriptorArray                    = true;
    vk12features.descriptorBindingVariableDescriptorCount  = true;
    vk12features.hostQueryReset                            = true;
    vk12features.shaderOutputLayer                         = true;
    vk12features.bufferDeviceAddress                       = true; // vuk requirement
    vk12features.vulkanMemoryModel                         = true; // general performance improvement
    vk12features.vulkanMemoryModelDeviceScope              = true; // general performance improvement
    VkPhysicalDeviceVulkan11Features vk11features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    vk11features.shaderDrawParameters = true;
    VkPhysicalDeviceSynchronization2FeaturesKHR sync_feat{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR, .synchronization2 = true};
    device_builder = device_builder.add_pNext(&vk12features).add_pNext(&vk11features).add_pNext(&sync_feat);
    auto dev_ret   = device_builder.build();
    assert_else(dev_ret.has_value());
    vkbdevice                        = dev_ret.value();
    graphics_queue                   = vkbdevice.get_queue(vkb::QueueType::graphics).value();
    auto graphics_queue_family_index = vkbdevice.get_queue_index(vkb::QueueType::graphics).value();
    transfer_queue                   = vkbdevice.get_queue(vkb::QueueType::transfer).value();
    auto transfer_queue_family_index = vkbdevice.get_queue_index(vkb::QueueType::transfer).value();
    device                           = vkbdevice.device;

    vuk::ContextCreateParameters::FunctionPointers fps;
#define VUK_EX_LOAD_FP(name) fps.name = (PFN_##name)vkGetDeviceProcAddr(device, #name);
    VUK_EX_LOAD_FP(vkSetDebugUtilsObjectNameEXT);
    VUK_EX_LOAD_FP(vkCmdBeginDebugUtilsLabelEXT);
    VUK_EX_LOAD_FP(vkCmdEndDebugUtilsLabelEXT);

    context.emplace(vuk::ContextCreateParameters{instance,
         device,
         physical_device,
         graphics_queue,
         graphics_queue_family_index,
         VK_NULL_HANDLE,
         VK_QUEUE_FAMILY_IGNORED,
         transfer_queue,
         transfer_queue_family_index,
        fps});
    const unsigned num_inflight_frames = 3;
    super_frame_resource.emplace(*context, num_inflight_frames);
    global_allocator.emplace(*super_frame_resource);
    swapchain = context->add_swapchain(make_swapchain(vkbdevice));
}

void Renderer::add_scene(RenderScene* scene) {
    scenes.push_back(scene);
    if (stage != RenderStage_Setup) {
        scene->setup(*global_allocator);
    }
}

void Renderer::setup() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsSpellbook();
    ImGui_ImplGlfw_InitForVulkan(window, false);
    imgui_data = ImGui_ImplVuk_Init(*global_allocator);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().ConfigDockingWithShift = true;

    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("src/shaders/depth_outline.comp"), "src/shaders/depth_outline.comp");
        game.renderer.context->create_named_pipeline("postprocess", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("src/shaders/standard_3d.vert"), "src/shaders/standard_3d.vert");
        pci.add_glsl(get_contents("src/shaders/textured_3d.frag"), "src/shaders/textured_3d.frag");
        context->create_named_pipeline("textured_model", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("src/shaders/infinite_plane.vert"), "src/shaders/infinite_plane.vert");
        pci.add_glsl(get_contents("src/shaders/grid.frag"), "src/shaders/grid.frag");
        game.renderer.context->create_named_pipeline("grid_3d", pci);
    }

    upload_defaults();

    {
        // OPTIMIZATION: can thread
        for (auto scene : scenes) {
            scene->setup(*global_allocator);
        }
    }

    wait_for_futures();

    stage = RenderStage_Inactive;
}

void Renderer::update() {
    ZoneScoped;
    while (suspend) {
        glfwWaitEvents();
    }
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    frame_timer.update();
}

void Renderer::render() {
    ZoneScoped;
    assert_else(stage == RenderStage_Inactive)
        return;

    wait_for_futures();

    stage = RenderStage_BuildingRG;

    auto& xdev_frame_resource = super_frame_resource->get_next_frame();
    context->next_frame();
    frame_allocator.emplace(xdev_frame_resource);
    std::shared_ptr<vuk::RenderGraph> rg = std::make_shared<vuk::RenderGraph>("renderer");

    std::vector resources{"SWAPCHAIN+"_image >> vuk::eColorWrite >> "SWAPCHAIN++"};
    for (auto scene : scenes) {
        scene->pre_render();

        std::shared_ptr<vuk::RenderGraph> rgx = std::make_shared<vuk::RenderGraph>(vuk::Name(scene->name));
        rgx->attach_image("input_uncleared", vuk::ImageAttachment::from_texture(scene->render_target));
        rgx->clear_image("input_uncleared", vuk::Name(scene->name + "_input"), vuk::ClearColor{0.1f, 0.1f, 0.1f, 1.0f});
        auto scene_fut     = scene->render(*frame_allocator, vuk::Future{rgx, vuk::Name(scene->name + "_input")});
        rg->attach_in(vuk::Name(scene->name + "_final"), std::move(scene_fut));

        resources.emplace_back(vuk::Resource {vuk::Name(scene->name + "_final"), vuk::Resource::Type::eImage, vuk::Access::eFragmentSampled});
    }

    ImGui::Render();
    // NOTE: When we render in 3D, we're using reverse depth. We have no need for that here because we don't have depth precision issues
    rg->clear_image("SWAPCHAIN", "SWAPCHAIN+", vuk::ClearColor{0.1f, 0.05f, 0.1f, 1.0f});
    rg->attach_swapchain("SWAPCHAIN", swapchain);
    
    rg->add_pass({.name = "force_transition", .resources = std::move(resources)});

    auto fut = ImGui_ImplVuk_Render(*frame_allocator, vuk::Future{rg, "SWAPCHAIN++"}, imgui_data, ImGui::GetDrawData(), imgui_images);
    stage    = RenderStage_Presenting;
    vuk::Compiler compiler;
    present(*frame_allocator, compiler, swapchain, std::move(fut));
    imgui_images.clear();

    for (auto scene : scenes) {
        for (auto it = scene->renderables.begin(); it != scene->renderables.end();) {
            if (it->frame_allocated)
                it = scene->renderables.erase(it);
            else
                ++it;
        }
    }
    
    frame_allocator.reset();

    stage = RenderStage_Inactive;
}

void Renderer::cleanup() {
    context->wait_idle();
    for (auto scene : scenes) {
        scene->cleanup(*global_allocator);
    }
    mesh_cache.clear();
    material_cache.clear();
    texture_cache.clear();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

Renderer::~Renderer() {
    imgui_data.font_texture.view.reset();
    imgui_data.font_texture.image.reset();
    super_frame_resource.reset();
    context.reset();
    vkDestroySurfaceKHR(vkbinstance.instance, surface, nullptr);
    destroy_window_glfw(window);
    vkb::destroy_device(vkbdevice);
    vkb::destroy_instance(vkbinstance);
}

void Renderer::wait_for_futures() {
    vuk::Compiler compiler;
    vuk::wait_for_futures_explicit(*global_allocator, compiler, futures);
    futures.clear();
}


void Renderer::resize(v2i new_size) {
    if (new_size == v2i(0,0)) {
        suspend = true;
        return;
    }
    suspend = false;
    context->wait_idle();
    global_allocator->deallocate(swapchain->image_views);
    global_allocator->deallocate({&swapchain->swapchain, 1});
    context->remove_swapchain(swapchain);
    auto new_swapchain = context->add_swapchain(make_swapchain(vkbdevice, swapchain));
    for (auto& iv : swapchain->image_views) {
        context->set_name(iv.payload, "Swapchain ImageView");
    }
    swapchain = new_swapchain;
}


string Renderer::upload_mesh(const MeshCPU& mesh_cpu, bool frame_allocation) {
    assert_else(!mesh_cpu.file_path.empty());
    u64 mesh_cpu_hash           = hash_data(mesh_cpu.file_path.data(), mesh_cpu.file_path.size());
    if (mesh_cache.contains(mesh_cpu_hash))
        return mesh_cpu.file_path;
    MeshGPU         mesh_gpu;
    vuk::Allocator& alloc                = frame_allocation ? *frame_allocator : *global_allocator;
    auto            [vert_buf, vert_fut] = create_buffer(alloc, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.vertices));
    mesh_gpu.vertex_buffer               = std::move(vert_buf);
    auto [idx_buf, idx_fut]              = create_buffer(alloc, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.indices));
    mesh_gpu.index_buffer                = std::move(idx_buf);
    mesh_gpu.index_count                 = mesh_cpu.indices.size();
    mesh_gpu.vertex_count                = mesh_cpu.vertices.size();

    enqueue_setup(std::move(vert_fut));
    enqueue_setup(std::move(idx_fut));

    mesh_cache[mesh_cpu_hash] = std::move(mesh_gpu);
    return mesh_cpu.file_path;
}

SkeletonGPU Renderer::upload_skeleton(const SkeletonCPU& skeleton_cpu) {
    SkeletonGPU skeleton_gpu;
    skeleton_gpu.bones = std::move(skeleton_cpu.bones);
    
    vuk::Allocator& alloc = *global_allocator;
    u32 alloc_size = sizeof(u32) * 4 + sizeof(m44GPU) * skeleton_gpu.bones.size();

    skeleton_gpu.buffer = *vuk::allocate_buffer(alloc, {vuk::MemoryUsage::eCPUtoGPU, alloc_size, 1});
    skeleton_gpu.update();
    return skeleton_gpu;
}

string Renderer::upload_material(const MaterialCPU& material_cpu, bool frame_allocation) {
    assert_else(!material_cpu.file_path.empty());
    u64 material_cpu_hash               = hash_data(material_cpu.file_path.data(), material_cpu.file_path.size());
    if (frame_allocation); // TODO: frame allocation

    MaterialGPU material_gpu;
    material_gpu.pipeline      = context->get_named_pipeline(vuk::Name(material_cpu.shader_name));
    material_gpu.color = vuk::make_sampled_image(get_texture_or_upload(material_cpu.color_asset_path).view.get(), material_cpu.sampler.get());
    material_gpu.normal = vuk::make_sampled_image(get_texture_or_upload(material_cpu.normal_asset_path).view.get(), material_cpu.sampler.get());
    material_gpu.orm = vuk::make_sampled_image(get_texture_or_upload(material_cpu.orm_asset_path).view.get(), material_cpu.sampler.get());
    material_gpu.emissive = vuk::make_sampled_image(get_texture_or_upload(material_cpu.emissive_asset_path).view.get(), material_cpu.sampler.get());

    material_gpu.tints         = {
        (v4) material_cpu.color_tint,
        (v4) material_cpu.emissive_tint,
        {material_cpu.roughness_factor, material_cpu.metallic_factor, material_cpu.normal_factor, 1.0f},
        {material_cpu.emissive_dot_smoothstep.x, material_cpu.emissive_dot_smoothstep.y, 0.0f, 0.0f}
    };
    material_gpu.cull_mode = material_cpu.cull_mode;

    material_cache[material_cpu_hash] = std::move(material_gpu);
    return material_cpu.file_path;
}

string Renderer::upload_texture(const TextureCPU& tex_cpu, bool frame_allocation) {
    assert_else(!tex_cpu.file_path.empty());
    u64 tex_cpu_hash = hash_data(tex_cpu.file_path.data(), tex_cpu.file_path.size());
    vuk::Allocator& alloc = frame_allocation ? *frame_allocator : *global_allocator;
    auto [tex, tex_fut] = create_texture(alloc, tex_cpu.format, vuk::Extent3D(tex_cpu.size), (void*) tex_cpu.pixels.data(), true);
    context->set_name(tex, vuk::Name(tex_cpu.file_path));
    enqueue_setup(std::move(tex_fut));

    if (frame_allocation); // TODO: frame allocation

    texture_cache[tex_cpu_hash] = std::move(tex);
    return tex_cpu.file_path;
}

MeshGPU* Renderer::get_mesh(const string& asset_path) {
    if (asset_path.empty())
        return nullptr;
    u64 hash = hash_data(asset_path.data(), asset_path.size());
    if (mesh_cache.count(hash))
        return &mesh_cache[hash];
    return nullptr;
}

MaterialGPU* Renderer::get_material(const string& asset_path) {
    if (asset_path.empty())
        return nullptr;
    u64 hash = hash_data(asset_path.data(), asset_path.size());
    if (material_cache.count(hash))
        return &material_cache[hash];
    return nullptr;
}

TextureGPU* Renderer::get_texture(const string& asset_path) {
    if (asset_path.empty())
        return nullptr;

    u64 hash = hash_data(asset_path.data(), asset_path.size());
    if (texture_cache.count(hash))
        return &texture_cache[hash];
    return nullptr;
}

MeshGPU& Renderer::get_mesh_or_upload(const string& asset_path) {
    assert_else(!asset_path.empty());
    u64 hash = hash_data(asset_path.data(), asset_path.size());
    if (mesh_cache.count(hash))
        return mesh_cache[hash];
    upload_mesh(load_mesh(asset_path));
    return mesh_cache[hash];
}

MaterialGPU& Renderer::get_material_or_upload(const string& asset_path) {
    assert_else(!asset_path.empty());
    u64 hash = hash_data(asset_path.data(), asset_path.size());
    if (material_cache.count(hash))
        return material_cache[hash];
    upload_material(load_material(asset_path));
    return material_cache[hash];
}

TextureGPU& Renderer::get_texture_or_upload(const string& asset_path) {
    assert_else(!asset_path.empty());
    u64 hash = hash_data(asset_path.data(), asset_path.size());
    if (texture_cache.count(hash))
        return texture_cache[hash];
    upload_texture(load_texture(asset_path));
    return texture_cache[hash];
}


void Renderer::debug_window(bool* p_open) {
    if (ImGui::Begin("Renderer", p_open)) {
        ImGui::Text(fmt_("Window Size: {}", window_size).c_str());

        frame_timer.inspect();

        if (ImGui::CollapsingHeader("Uploaded Meshes")) {

        }
        if (ImGui::CollapsingHeader("Uploaded Materials")) {

        }
        if (ImGui::CollapsingHeader("Uploaded Textures")) {

        }
    }
    ImGui::End();
}


void Renderer::upload_defaults() {
    TextureCPU tex_white_upload {
        .file_path = "textures/white.sbtex",
        .size = v2i(8, 8),
        .format = vuk::Format::eR8G8B8A8Srgb,
        .pixels = vector<u8>(8 * 8 * 4, 255)
    };
    upload_texture(tex_white_upload);

    constexpr u32 grid_size = 2048;
    TextureCPU tex_grid_upload {
        .file_path = "textures/grid.sbtex",
        .size = v2i(grid_size, grid_size),
        .format = vuk::Format::eR8G8B8A8Srgb,
        .pixels = vector<u8>(grid_size * grid_size * 4, 255)
    };
    // do border
    for (u32 i = 0; i < (grid_size - 1); i++) {
        for (u32 pixel_pos : vector<u32>{i, i * grid_size + (grid_size - 1), (grid_size - 1) * grid_size + i + 1, i * grid_size + grid_size}) {
            tex_grid_upload.pixels[pixel_pos * 4 + 0] = 0;
            tex_grid_upload.pixels[pixel_pos * 4 + 1] = 0;
            tex_grid_upload.pixels[pixel_pos * 4 + 2] = 0;
        }
    }
    upload_texture(tex_grid_upload);

    MaterialCPU default_mat = {
        .file_path = "default",
        .color_tint = palette::black,
    };
    upload_material(default_mat);
    TextureCPU default_tex = {
        .file_path = "default",
        .size = {2, 2},
        .format = vuk::Format::eR8G8B8A8Srgb,
        .pixels = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255}
    };
    upload_texture(default_tex);
    MeshCPU default_mesh   = generate_cube(v3(0), v3(1));
    default_mesh.file_path = "default";
    upload_mesh(default_mesh);
}


void FrameTimer::update() {
    int last_index   = ptr;
    f32 last_time    = frame_times[ptr];
    ptr              = (ptr + 1) % 200;
    frame_times[ptr] = Input::time;

    if (filled > 1)
        delta_times[last_index] = frame_times[ptr] - last_time;
    filled = math::min(filled + 1, 200);
}

void FrameTimer::inspect() {
    f32 average = 0.0f;
    for (int n = 0; n < filled; n++)
        average += delta_times[n];
    average /= (f32) filled;
    string overlay = fmt_("FPS: {}", 1.0f / average);
    ImGui::PlotLines("DT", delta_times.data(), filled, ptr, overlay.c_str(), 0.0f, 0.1f, ImVec2(0, 80.0f));
}

}
