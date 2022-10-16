#include "renderer.hpp"

#include <vuk/src/RenderGraphUtil.hpp>
#include <vuk/Partials.hpp>
#include <backends/imgui_impl_glfw.h>
#include <tracy/Tracy.hpp>

#include "lib_ext/fmt_geometry.hpp"

#include "file.hpp"
#include "matrix_math.hpp"
#include "console.hpp"
#include "draw_functions.hpp"
#include "game.hpp"
#include "render_scene.hpp"
#include "samplers.hpp"

#include "utils.hpp"
#include "assets/texture_asset.hpp"

#include "stb_image.h"
#include "assets/mesh_asset.hpp"

namespace spellbook {

Renderer::Renderer() : imgui_data() {
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
        .set_app_name("cargo_container")
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

    GLFWimage images[1]; 
    images[0].pixels = stbi_load("icon.png", &images[0].width, &images[0].height, 0, 4); //rgba channels 
    glfwSetWindowIcon(window, 1, images); 
    stbi_image_free(images[0].pixels);
    
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

    context.emplace(vuk::ContextCreateParameters{instance,
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
    
    upload_defaults();
    
    {
        // OPTIMIZATION: can thread
        for (auto scene : scenes) {
            scene->setup(*global_allocator);
        }
    }

    wait_for_futures();
    
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
    assert_else(stage == RenderStage_Inactive)
        return;

    wait_for_futures();
    
    stage = RenderStage_BuildingRG;

    auto& xdev_frame_resource = super_frame_resource->get_next_frame();
    context->next_frame();
    frame_allocator.emplace(xdev_frame_resource);
    std::shared_ptr<vuk::RenderGraph> rg = std::make_shared<vuk::RenderGraph>("renderer");
    plf::colony<vuk::Name>            attachment_names;

    for (auto& callback : start_render_callbacks) {
        callback();
    }
    
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
            {.extent = vuk::Dimension3D::absolute((u32) output_size.x, (u32) output_size.y),
             .format = vuk::Format::eR16G16B16A16Sfloat,
             .sample_count = vuk::Samples::e1,
             .level_count = 1,
             .layer_count = 1},
            (vuk::ClearColor) Color(0.07f, 0.06f, 0.07f, 0.0f));

        auto       rg_frag_fut         = scene->render(*frame_allocator, vuk::Future{rgx, "_img"});
        vuk::Name& attachment_name_out = *attachment_names.emplace(std::string(scene->name) + "_final");
        auto       rg_frag             = rg_frag_fut.get_render_graph();
        vuk::Compiler compiler;
        compiler.compile({&rg_frag, 1}, {});
        rg->attach_in(attachment_name_out, std::move(rg_frag_fut));
        auto si = vuk::make_sampled_image(rg->name.append("::").append(attachment_name_out.to_sv()), imgui_data.font_sci);
        ImGui::Image(&*sampled_images.emplace(si), ImGui::GetContentRegionAvail());
        ImGui::PopStyleVar();
        ImGui::End();
    }

    ImGui::Render();
    // NOTE: When we render in 3D, we're using reverse depth. We have no need for that here because we don't have depth precision issues
    rg->clear_image("SWAPCHAIN", "SWAPCHAIN+", vuk::ClearColor{0.4f, 0.2f, 0.4f, 1.0f});
    rg->attach_swapchain("SWAPCHAIN", swapchain);
    auto fut = ImGui_ImplVuk_Render(*frame_allocator, vuk::Future{rg, "SWAPCHAIN+"}, imgui_data, ImGui::GetDrawData(), sampled_images);
    stage    = RenderStage_Presenting;
    vuk::Compiler compiler;
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

void Renderer::wait_for_futures() {
    vuk::Compiler compiler;
    vuk::wait_for_futures_explicit(*global_allocator, compiler, futures);
    futures.clear();
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
    assert_else(!mesh_cpu.file_name.empty());
    u64 mesh_cpu_hash           = hash_data(mesh_cpu.file_name.data(), mesh_cpu.file_name.size());
    mesh_aliases[mesh_cpu.name] = mesh_cpu_hash;
    MeshGPU         mesh_gpu;
    vuk::Allocator& alloc                = frame_allocation ? *frame_allocator : *global_allocator;
    auto            [vert_buf, vert_fut] = create_buffer_gpu(alloc, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.vertices));
    mesh_gpu.vertex_buffer               = std::move(vert_buf);
    auto [idx_buf, idx_fut]              = create_buffer_gpu(alloc, vuk::DomainFlagBits::eTransferOnTransfer, std::span(mesh_cpu.indices));
    mesh_gpu.index_buffer                = std::move(idx_buf);
    mesh_gpu.index_count                 = (u32) mesh_cpu.indices.size();
    mesh_gpu.vertex_count                = (u32) mesh_cpu.vertices.size();
    enqueue_setup(std::move(vert_fut));
    enqueue_setup(std::move(idx_fut));

    if (frame_allocation); // TODO: frame allocation

    mesh_cache[mesh_cpu_hash] = std::move(mesh_gpu);
    return mesh_cache[mesh_cpu_hash];
}

MaterialGPU& Renderer::upload_material(const MaterialCPU& material_cpu, bool frame_allocation) {
    assert_else(!material_cpu.file_name.empty());
    u64 material_cpu_hash               = hash_data(material_cpu.file_name.data(), material_cpu.file_name.size());
    material_aliases[material_cpu.name] = material_cpu_hash;
    if (frame_allocation); // TODO: frame allocation

    MaterialGPU material_gpu;
    material_gpu.pipeline      = context->get_named_pipeline("textured_model");
    material_gpu.color_view    = get_texture_or_upload(material_cpu.color_asset_path).view.get();
    material_gpu.orm_view      = get_texture_or_upload(material_cpu.orm_asset_path).view.get();
    material_gpu.normal_view   = get_texture_or_upload(material_cpu.normal_asset_path).view.get();
    material_gpu.emissive_view = get_texture_or_upload(material_cpu.emissive_asset_path).view.get();
    material_gpu.tints         = {
        (v4) material_cpu.color_tint,
        (v4) material_cpu.emissive_tint,
        {material_cpu.roughness_factor, material_cpu.metallic_factor, material_cpu.normal_factor, material_cpu.uv_scale}
    };
    material_gpu.cull_mode = material_cpu.cull_mode;

    material_cache[material_cpu_hash] = std::move(material_gpu);
    return material_cache[material_cpu_hash];
}

TextureGPU& Renderer::upload_texture(const TextureCPU& tex_cpu, bool frame_allocation) {
    assert_else(!tex_cpu.file_name.empty());
    u64 tex_cpu_hash              = hash_data(tex_cpu.file_name.data(), tex_cpu.file_name.size());
    texture_aliases[tex_cpu.name] = tex_cpu_hash;
    vuk::Allocator& alloc = frame_allocation ? *frame_allocator : *global_allocator;
    auto [tex, tex_fut] = create_texture(alloc, tex_cpu.format, vuk::Extent3D(tex_cpu.size), (void*) tex_cpu.pixels.data(), true);
    context->debug.set_name(tex, vuk::Name(tex_cpu.name));
    enqueue_setup(std::move(tex_fut));

    if (frame_allocation); // TODO: frame allocation

    texture_cache[tex_cpu_hash] = std::move(tex);
    return texture_cache[tex_cpu_hash];
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
    return upload_mesh(load_mesh(asset_path));
}

MaterialGPU& Renderer::get_material_or_upload(const string& asset_path) {
    assert_else(!asset_path.empty());
    u64 hash = hash_data(asset_path.data(), asset_path.size());
    if (material_cache.count(hash))
        return material_cache[hash];
    return upload_material(load_material(asset_path));
}

TextureGPU& Renderer::get_texture_or_upload(const string& asset_path) {
    assert_else(!asset_path.empty());
    u64 hash = hash_data(asset_path.data(), asset_path.size());
    if (texture_cache.count(hash))
        return texture_cache[hash];
    return upload_texture(load_texture(asset_path));
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
    if (true || !file_exists("textures/white.sbtex")) {
        TextureCPU tex_white_upload = convert_to_texture("external_resources/images/white.jpg", "textures", "white");
        save_texture(tex_white_upload);
    }
    upload_texture(load_texture("textures/white.sbtex"));

    if (true || !file_exists("textures/grid.sbtex")) {
        TextureCPU tex_white_upload = convert_to_texture("external_resources/images/grid.png", "textures", "grid");
        save_texture(tex_white_upload);
    }
    upload_texture(load_texture("textures/grid.sbtex"));
    
    MaterialCPU default_mat = {
        .name = "default",
        .file_name = "default",
        .color_tint = palette::black,
    };
    upload_material(default_mat);
    TextureCPU default_tex = {
        .name = "default",
        .file_name = "default",
        .size = {2,2},
        .format = vuk::Format::eR8G8B8A8Srgb,
        .pixels = {255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255}
    };
    upload_texture(default_tex);
    MeshCPU default_mesh = generate_cube(v3(0), v3(1));
    default_mesh.name = "default";
    default_mesh.file_name = "default";
    upload_mesh(default_mesh);
}


void Renderer::generate_thumbnail(const ModelCPU& model, const string& name) {
    static vuk::SampledImage* p_image = nullptr;
    static bool initialized = false;

    if (!initialized) {
        struct CameraData {
            m44GPU view;
            m44GPU proj;
            v4     position;
        } cam_data;
        v3 cam_position = v3(2);
        cam_data.view     = (m44GPU) math::look(cam_position, -cam_position, v3(0,0,1));
        cam_data.proj     = (m44GPU) math::infinite_perspective(math::PI / 2.f, 1.0f, 0.1f);
        cam_data.position = v4(cam_position, 1.0f);

        auto [pubo_camera, fubo_camera] = vuk::create_buffer_cross_device(*frame_allocator, vuk::MemoryUsage::eCPUtoGPU, std::span(&cam_data, 1));

        struct SceneData {
            v4 ambient = v4(1.f, 1.f, 1.f, 0.4f);
            v4 fog = v4(0.04f, 0.02f, 0.04f, -1.f);
            v4 sun_data = v4(0.5, 0.5, 0.5, 1.0f);
            v2 rim_intensity_start = v2(0.25f, 0.55f);
        } scene_data_gpu;

        auto [pubo_scene, fubo_scene] = vuk::create_buffer_cross_device(*frame_allocator, vuk::MemoryUsage::eCPUtoGPU, std::span(&scene_data_gpu, 1));

        RenderScene render_scene;
        ModelGPU model_gpu = instance_model(render_scene, model);
        
        auto buffer_model_mats = **vuk::allocate_buffer_cross_device(*frame_allocator, {vuk::MemoryUsage::eCPUtoGPU, sizeof(m44GPU) * model_gpu.renderables.size(), 1});
        int i = 0;
        for (const auto& slot : model_gpu.renderables) {
            auto transform_gpu = m44GPU(render_scene.renderables[slot].transform);
            memcpy(reinterpret_cast<m44GPU*>(buffer_model_mats.mapped_ptr) + i++, &transform_gpu, sizeof(m44GPU));
        }

        auto rg = make_shared<vuk::RenderGraph>("graph");
        
        // Set up the pass to draw the textured cube, with a color and a depth attachment
        // clang-format off
        rg->add_pass({
            .name = "forward",
            .resources = {
                "forward_input"_image >> vuk::eColorWrite     >> "forward_output",
                "normal_input"_image  >> vuk::eColorWrite     >> "normal_output",
                "depth_input"_image   >> vuk::eDepthStencilRW >> "depth_output"
            },
            .execute = [this, &pubo_camera, &pubo_scene, &buffer_model_mats, &model_gpu, &render_scene](vuk::CommandBuffer& command_buffer) {
                ZoneScoped;
                // Prepare render
                command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
                    .set_viewport(0, vuk::Rect2D::framebuffer())
                    .set_scissor(0, vuk::Rect2D::framebuffer())
                    .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                              .depthTestEnable  = true,
                              .depthWriteEnable = true,
                              .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
                    })
                    .broadcast_color_blend({vuk::BlendPreset::eAlphaBlend})
                    .set_color_blend("forward_input", vuk::BlendPreset::eAlphaBlend)
                    .set_color_blend("normal_input", vuk::BlendPreset::eOff);

                // Bind buffers
                command_buffer
                    .bind_buffer(0, 0, *pubo_camera)
                    .bind_buffer(0, 1, *pubo_scene)
                    .bind_buffer(0, 2, buffer_model_mats);
                int  i           = 0;
                auto render_item = [&](Renderable& renderable) {
                    ZoneScoped;
                    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_asset_path);
                    MaterialGPU* material = game.renderer.get_material(renderable.material_asset_path);
                    if (mesh == nullptr || material == nullptr) {
                        i++;
                        return;
                    }
                    // Bind mesh
                    command_buffer
                        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, vuk::Packed {
                            vuk::Format::eR32G32B32Sfloat, // position
                            vuk::Format::eR32G32B32Sfloat, // normal
                            vuk::Format::eR32G32B32Sfloat, // tangent
                            vuk::Format::eR32G32B32Sfloat, // color
                            vuk::Format::eR32G32Sfloat     // uv
                        })
                        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);

                    // Bind Material
                    command_buffer
                        .set_rasterization({.cullMode = material->cull_mode})
                        .bind_graphics_pipeline(material->pipeline);
                    material->bind_parameters(command_buffer);
                    material->bind_textures(command_buffer);

                    // Draw call
                    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, i++);
                };

                // Render items
                for (auto slot : model_gpu.renderables) {
                    render_item(render_scene.renderables[slot]);
                }
                // Render grid
                auto grid_view = game.renderer.get_texture("textures/grid.sbtex")->view.get();
                command_buffer
                    .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                        .depthTestEnable  = true,
                        .depthWriteEnable = false,
                        .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
                    })
                    .bind_graphics_pipeline("grid_3d")
                    .set_rasterization({})
                    .bind_buffer(0, 0, *pubo_camera)
                    .bind_image(0, 1, grid_view)
                    .bind_sampler(0, 1, TrilinearAnisotropic)
                    .draw(6, 1, 0, 0);
            }});
        rg->add_pass(vuk::Pass {
            .name = "postprocess_apply",
            .resources = {
                "forward_output"_image >> vuk::eComputeSampled,
                "normal_output"_image  >> vuk::eComputeSampled,
                "depth_output"_image   >> vuk::eComputeSampled,
                "target_input"_image   >> vuk::eComputeWrite >> "target_output",
            },
            .execute =
                [this](vuk::CommandBuffer& cmd) {
                    cmd
                        .bind_compute_pipeline("postprocess")
                        .bind_image(0, 0, "forward_output")
                        .bind_sampler(0, 0, NearestClamp)
                        .bind_image(0, 1, "normal_output")
                        .bind_sampler(0, 1, NearestClamp)
                        .bind_image(0, 2, "depth_output")
                        .bind_sampler(0, 2, NearestClamp)
                        .bind_image(0, 3, "target_input");

                    auto target      = *cmd.get_resource_image_attachment("target_input");
                    auto target_size = target.extent.extent;
                    cmd.specialize_constants(0, target_size.width);
                    cmd.specialize_constants(1, target_size.height);

                    cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, v4(5.0f, 6.0f, 0.05f, 3.0f));

                    cmd.dispatch_invocations(target_size.width, target_size.height);
                },
        });
        // clang-format on

        auto clear_color      = vuk::ClearColor {scene_data_gpu.fog.r, scene_data_gpu.fog.g, scene_data_gpu.fog.b, 1.0f};
        auto depth_clear_value = vuk::ClearDepthStencil{0.0f, 0};
        rg->attach_and_clear_image("forward_input", {.format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1}, clear_color);
        rg->attach_and_clear_image("normal_input", {.format = vuk::Format::eR16G16B16A16Sfloat}, clear_color);
        rg->attach_and_clear_image("depth_input", {.format = vuk::Format::eD32Sfloat}, depth_clear_value);
        rg->attach_and_clear_image("target_input",
            {.extent = vuk::Dimension3D::absolute(100u, 100u),
             .format = vuk::Format::eR16G16B16A16Sfloat,
             .sample_count = vuk::Samples::e1,
             .level_count = 1,
             .layer_count = 1},
            (vuk::ClearColor) Color(0.07f, 0.06f, 0.07f, 0.0f));
        
        rg->inference_rule("forward_input", vuk::same_extent_as("target_input"));

        vuk::Compiler compiler;
        compiler.compile({&rg, 1}, {});
        p_image = &*sampled_images.emplace(vuk::make_sampled_image(rg->name.append("::").append("target_output"), NearestClamp));
        initialized = true;
    }
    
    ImGui::Image(p_image, ImVec2(100, 100));
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
