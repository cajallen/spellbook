﻿#include "renderer.hpp"

#include <vuk/src/RenderGraphUtil.hpp>
#include <vuk/Partials.hpp>
#include <backends/imgui_impl_glfw.h>
#include <tracy/Tracy.hpp>

#include "lib_ext/fmt_geometry.hpp"

#include "file.hpp"
#include "matrix_math.hpp"
#include "console.hpp"
#include "render_scene.hpp"

#include "utils.hpp"
#include "assets/texture_asset.hpp"

namespace spellbook {

Renderer::Renderer() {
    vkb::InstanceBuilder builder;
    builder
#if VALIDATION
        .request_validation_layers()
        .set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                                VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                void*                                       pUserData) -> VkBool32 {
            auto ms = vkb::to_string_message_severity(messageSeverity);
            auto mt = vkb::to_string_message_type(messageType);
            console({.str = fmt_("[{}: {}]\n{}\n", ms, mt, pCallbackData->pMessage), .group = "vulkan", .color = palette::crimson});
            __debugbreak();
            return VK_FALSE;
        })
#endif
        .set_app_name("cargo_container")
        .set_engine_name("spellbook")
        .require_api_version(1, 2, 0)
        .set_app_version(0, 1, 0);
    auto inst_ret = builder.build();
    assert_else(inst_ret.has_value()) return;
    vkbinstance                          = inst_ret.value();
    auto                        instance = vkbinstance.instance;
    vkb::PhysicalDeviceSelector selector {vkbinstance};
    VkPhysicalDeviceFeatures    vkfeatures {
        .independentBlend = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };
    window                       = create_window_glfw("Spellbook", window_size, true);
    surface                      = create_surface_glfw(vkbinstance.instance, window);
    selector.set_surface(surface)
        .set_minimum_version(1, 0)
        .set_required_features(vkfeatures)
        .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    auto phys_ret = selector.select();
    assert_else(phys_ret.has_value()) {
        // error
    }
    vkb::PhysicalDevice vkbphysical_device = phys_ret.value();
    physical_device                        = vkbphysical_device.physical_device;

    vkb::DeviceBuilder               device_builder {vkbphysical_device};
    VkPhysicalDeviceVulkan12Features vk12features {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
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
    VkPhysicalDeviceVulkan11Features vk11features {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    vk11features.shaderDrawParameters = true;
    VkPhysicalDeviceSynchronization2FeaturesKHR sync_feat {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR, .synchronization2 = true};
    device_builder = device_builder.add_pNext(&vk12features).add_pNext(&vk11features).add_pNext(&sync_feat);
    auto dev_ret   = device_builder.build();
    assert_else(dev_ret.has_value()) {
        // error
    }
    vkbdevice                        = dev_ret.value();
    graphics_queue                   = vkbdevice.get_queue(vkb::QueueType::graphics).value();
    auto graphics_queue_family_index = vkbdevice.get_queue_index(vkb::QueueType::graphics).value();
    transfer_queue                   = vkbdevice.get_queue(vkb::QueueType::transfer).value();
    auto transfer_queue_family_index = vkbdevice.get_queue_index(vkb::QueueType::transfer).value();
    device                           = vkbdevice.device;

    vuk::ContextCreateParameters::FunctionPointers fps;

    context.emplace(vuk::ContextCreateParameters {instance,
        device,
        physical_device,
        graphics_queue,
        graphics_queue_family_index,
        VK_NULL_HANDLE,
        VK_QUEUE_FAMILY_IGNORED,
        transfer_queue,
        transfer_queue_family_index});
    const unsigned num_inflight_frames = 3;
    super_frame_resource.emplace(*context, num_inflight_frames);
    global_allocator.emplace(*super_frame_resource);
    swapchain = context->add_swapchain(make_swapchain(vkbdevice));
}

void Renderer::add_scene(RenderScene* scene) {
    scenes.insert_back(scene);
    if (setup_finished) {
        scene->setup(*global_allocator);
    }
}

void Renderer::setup() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(window, false);
    imgui_data = ImGui_ImplVuk_Init(*global_allocator);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().ConfigDockingWithShift = true;

    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("src/shaders/standard_3d.vert"), "standard_3d.vert");
        pci.add_glsl(get_contents("src/shaders/textured_3d.frag"), "textured_3d.frag");
        context->create_named_pipeline("textured_model", pci);
    }

    // TODO: get white and grid textures
    {
        // TODO: can thread
        for (auto scene : scenes) {
            scene->setup(*global_allocator);
        }
    }

    vuk::Compiler compiler;
    vuk::wait_for_futures_explicit(*global_allocator, compiler, futures);
    futures.clear();
    setup_finished = true;
}

void Renderer::update() {
    ZoneScoped;
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    frame_timer.update();
}

void Renderer::render() {
    ZoneScoped;
    assert_else(stage == RenderStage_Inactive) return;

    vuk::Compiler compiler;
    vuk::wait_for_futures_explicit(*global_allocator, compiler, futures);
    futures.clear();
    stage = RenderStage_BuildingRG;

    auto& xdev_frame_resource = super_frame_resource->get_next_frame();
    context->next_frame();
    frame_allocator.emplace(xdev_frame_resource);
    std::shared_ptr<vuk::RenderGraph> rg = std::make_shared<vuk::RenderGraph>("renderer");
    plf::colony<vuk::Name>            attachment_names;

    size_t i = 0;
    for (auto scene : scenes) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::Begin(scene->name.c_str());
        v2i output_size       = (v2i) ImGui::GetContentRegionAvail();
        output_size.x         = output_size.x <= 0 ? 1 : output_size.x;
        output_size.y         = output_size.y <= 0 ? 1 : output_size.y;
        scene->viewport.start = (v2i) ImGui::GetWindowPos() + (v2i) ImGui::GetCursorPos();
        scene->viewport.update_size(output_size);
        scene->viewport.window_hovered = ImGui::IsWindowHovered();
        scene->viewport.pre_render(); // prepares matrices

        std::shared_ptr<vuk::RenderGraph> rgx = std::make_shared<vuk::RenderGraph>(vuk::Name(scene->name));
        rgx->attach_and_clear_image("_img",
            {.extent          = vuk::Dimension3D::absolute((u32) output_size.x, (u32) output_size.y),
                .format       = vuk::Format::eR16G16B16A16Sfloat,
                .sample_count = vuk::Samples::e1,
                .level_count  = 1,
                .layer_count  = 1},
            (vuk::ClearColor) Color(0.07f, 0.06f, 0.07f, 0.0f));

        auto       rg_frag_fut         = scene->render(*frame_allocator, vuk::Future {rgx, "_img"});
        vuk::Name& attachment_name_out = *attachment_names.emplace(std::string(scene->name) + "_final");
        auto       rg_frag             = rg_frag_fut.get_render_graph();
        compiler.compile({&rg_frag, 1}, {});
        rg->attach_in(attachment_name_out, std::move(rg_frag_fut));
        auto si = vuk::make_sampled_image(rg->name.append("::").append(attachment_name_out.to_sv()), imgui_data.font_sci);
        ImGui::Image(&*sampled_images.emplace(si), ImGui::GetContentRegionAvail());
        ImGui::PopStyleVar();
        ImGui::End();
    }

    ImGui::Render();
    // NOTE: When we render in 3D, we're using reverse depth. We have no need for that here because we don't have depth precision issues
    rg->clear_image("SWAPCHAIN", "SWAPCHAIN+", vuk::ClearColor {0.4f, 0.2f, 0.4f, 1.0f});
    rg->attach_swapchain("SWAPCHAIN", swapchain);
    auto fut = ImGui_ImplVuk_Render(*frame_allocator, vuk::Future {rg, "SWAPCHAIN+"}, imgui_data, ImGui::GetDrawData(), sampled_images);
    stage    = RenderStage_Presenting;
    present(*frame_allocator, compiler, swapchain, std::move(fut));

    sampled_images.clear();
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

void Renderer::resize(v2i new_size) {
    context->wait_idle();
    auto new_swapchain = context->add_swapchain(make_swapchain(vkbdevice, swapchain));
    global_allocator->deallocate(swapchain->image_views);
    global_allocator->deallocate({&swapchain->swapchain, 1});
    context->remove_swapchain(swapchain);
    swapchain = new_swapchain;
}

MeshGPU& Renderer::upload_mesh(const MeshCPU& mesh_cpu, bool frame_allocation) {
    u64 mesh_cpu_hash = mesh_cpu.contents_hash();
    mesh_aliases[mesh_cpu.name] = mesh_cpu_hash;
    if (mesh_cache.contains(mesh_cpu_hash))
        return mesh_cache[mesh_cpu_hash];
    
    MeshGPU         mesh_gpu;
    vuk::Allocator& alloc     = frame_allocation ? *frame_allocator : *global_allocator;
    auto [vert_buf, vert_fut] = create_buffer_gpu(alloc, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.vertices));
    mesh_gpu.vertex_buffer    = std::move(vert_buf);
    auto [idx_buf, idx_fut]   = create_buffer_gpu(alloc, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.indices));
    mesh_gpu.index_buffer     = std::move(idx_buf);
    mesh_gpu.index_count      = (u32) mesh_cpu.indices.size();
    mesh_gpu.vertex_count     = (u32) mesh_cpu.vertices.size();
    enqueue_setup(std::move(vert_fut));
    enqueue_setup(std::move(idx_fut));
    
    if (frame_allocation); // TODO: frame allocation
    
    mesh_cache[mesh_cpu_hash] = std::move(mesh_gpu);
    return mesh_cache[mesh_cpu_hash];
}

MaterialGPU& Renderer::upload_material(const MaterialCPU& material_cpu, bool frame_allocation) {
    u64 material_cpu_hash = material_cpu.contents_hash();
    material_aliases[material_cpu.name] = material_cpu_hash;
    if (material_cache.contains(material_cpu_hash))
        return material_cache[material_cpu_hash];

    if (frame_allocation); // TODO: frame allocation

    MaterialGPU material_gpu;
    material_gpu.pipeline                = context->get_named_pipeline("textured_model");
    material_gpu.base_color_view         = find_texture(material_cpu.base_color_texture)->view.get();
    material_gpu.metallic_roughness_view = find_texture(material_cpu.orm_texture)->view.get();
    material_gpu.normal_view             = find_texture(material_cpu.normal_texture)->view.get();
    material_gpu.emissive_view           = find_texture(material_cpu.emissive_texture)->view.get();
    material_gpu.tints                   = {
        (v4) material_cpu.base_color_tint,
        (v4) material_cpu.emissive_tint,
        {material_cpu.roughness_factor, material_cpu.metallic_factor, material_cpu.normal_factor, material_cpu.uv_scale}
    };
    material_gpu.cull_mode               = material_cpu.cull_mode;
    
    material_cache[material_cpu_hash] = std::move(material_gpu);
    return material_cache[material_cpu_hash];
}

TextureGPU& Renderer::upload_texture(const TextureCPU& tex_cpu, bool frame_allocation) {
    u64 tex_cpu_hash = tex_cpu.contents_hash();
    texture_aliases[tex_cpu.name] = tex_cpu_hash;
    if (texture_cache.contains(tex_cpu_hash))
        return texture_cache[tex_cpu_hash];
    vuk::Allocator& alloc = frame_allocation ? *frame_allocator : *global_allocator;
    auto [tex, tex_fut]   = create_texture(alloc, tex_cpu.format, vuk::Extent3D(tex_cpu.size), (void*) tex_cpu.pixels.data(), true);
    context->debug.set_name(tex, vuk::Name(tex_cpu.name));
    enqueue_setup(std::move(tex_fut));

    if (frame_allocation);  // TODO: frame allocation

    texture_cache[tex_cpu_hash] = std::move(tex);
    return texture_cache[tex_cpu_hash];
}

MeshGPU* Renderer::find_mesh(const string& name) {
    if (mesh_aliases.count(name))
        return &mesh_cache[mesh_aliases[name]];
    return nullptr;
}
MaterialGPU* Renderer::find_material(const string& name) {
    if (material_aliases.count(name))
        return &material_cache[material_aliases[name]];
    return nullptr;
}
TextureGPU* Renderer::find_texture(const string& name) {
    if (texture_aliases.count(name))
        return &texture_cache[texture_aliases[name]];
    return nullptr;
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

void FrameTimer::update() {
    int last_index = ptr;
    f32 last_time = frame_times[ptr];
    ptr = (ptr + 1) % 200;
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
