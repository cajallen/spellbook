#include "render_scene.hpp"

#include <functional>
#include <tracy/Tracy.hpp>
#include <vuk/Partials.hpp>

#include "draw_functions.hpp"
#include "extension/fmt_renderer.hpp"
#include "general/file.hpp"
#include "general/matrix_math.hpp"
#include "game/game.hpp"
#include "editor/console.hpp"
#include "editor/pose_widget.hpp"
#include "renderer/samplers.hpp"
#include "renderer/viewport.hpp"
#include "renderer/renderable.hpp"
#include "renderer/assets/mesh_asset.hpp"

namespace vuk {
static Texture allocate_texture(Allocator& allocator, Format format, Extent3D extent) {
    ImageCreateInfo ici;
    ici.format = format;
    ici.extent = extent;
    ici.samples = Samples::e1;
    ici.initialLayout = ImageLayout::eUndefined;
    ici.tiling        = ImageTiling::eOptimal;
    ici.usage         = ImageUsageFlagBits::eStorage | ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eSampled;
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    auto tex = allocator.get_context().allocate_texture(allocator, ici);
    return std::move(tex);
}
}


namespace spellbook {

void RenderScene::setup(vuk::Allocator& allocator) {
}

void RenderScene::image(v2i size) {
    viewport.start = (v2i) ImGui::GetWindowPos() + (v2i) ImGui::GetCursorPos();
    update_size(math::max(size, v2i(2, 2)));

    auto si = vuk::make_sampled_image(render_target.view.get(), Sampler().get());
    ImGui::Image(&*game.renderer.imgui_images.emplace(si), ImGui::GetContentRegionAvail());
}

void RenderScene::settings_gui() {
    ImGui::Checkbox("Pause", &user_pause);
    if (ImGui::TreeNode("Lighting")) {
        ImGui::ColorEdit4("Ambient", scene_data.ambient.data);
        ImGui::DragFloat3("rim_alpha_width_start", scene_data.rim_alpha_width_start.data, 0.01f);

        u32 id = ImGui::GetID("Sun Direction");
        PoseWidgetSettings settings {.render_scene = *this, .disabled = 0b1 << Operation_RotateZ};
        pose_widget(id, nullptr, &scene_data.sun_direction, settings);
        ImGui::DragFloat("Sun Intensity", &scene_data.sun_intensity, 0.01f);
        ImGui::DragFloat2("Outline(D)", &post_process_data.outline.x, 0.1f);
        ImGui::DragFloat2("Outline(N)", &post_process_data.outline.z, 0.002f);
        ImGui::EnumCombo("Debug Mode", &post_process_data.debug_mode);
        ImGui::TreePop();
    }
    ImGui::Text("Viewport");
    inspect(&viewport);
}

void RenderScene::update_size(v2i new_size) {
    render_target = vuk::allocate_texture(*game.renderer.global_allocator, vuk::Format::eB8G8R8A8Unorm, vuk::Extent3D(new_size));
    viewport.update_size(new_size);
}

Renderable* RenderScene::add_renderable(Renderable&& renderable) {
    // console({.str = fmt_("Adding renderable: {}", renderable), .group = "renderables"});
    return &*renderables.emplace(std::move(renderable));
}
Renderable* RenderScene::add_renderable(const Renderable& renderable) {
    // console({.str = fmt_("Adding renderable: {}", renderable), .group = "renderables"});
    return &*renderables.emplace(renderable);
}
Renderable* RenderScene::copy_renderable(Renderable* renderable) {
    // console({.str = fmt_("Copying renderable: {}", copy), .group = "renderables"});
    return &*renderables.emplace(*renderable);
}

void RenderScene::delete_renderable(Renderable* renderable) {
    // console({.str = fmt_("Deleting renderable"), .group = "renderables"});
    renderables.erase(renderables.get_iterator(renderable));
}


void RenderScene::_upload_buffer_objects(vuk::Allocator& allocator) {
    ZoneScoped;
    
    struct CameraData {
        m44GPU vp;
    };
    CameraData cam_data;
    cam_data.vp     = (m44GPU) viewport.camera->vp;
    CameraData sun_cam_data;
    v3 sun_vec = math::normalize(math::rotate(scene_data.sun_direction, v3::Z));
    sun_cam_data.vp     = (m44GPU) (math::orthographic(v3(8.0f, 8.0f, 20.0f)) * math::look(sun_vec * 10.0f, -sun_vec, v3::Z));

    auto [pubo_camera, fubo_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&cam_data, 1));
    buffer_camera_data              = *pubo_camera;
    
    auto [pubo_sun_camera, fubo_sun_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&sun_cam_data, 1));
    buffer_sun_camera_data              = *pubo_sun_camera;
    
    buffer_model_mats = **vuk::allocate_buffer(allocator, {vuk::MemoryUsage::eCPUtoGPU, sizeof(m44GPU) * (renderables.size() + widget_renderables.size()), 1});
    int i = 0;
    for (const auto& renderable : renderables) {
        memcpy((m44GPU*) buffer_model_mats.mapped_ptr + i++, &renderable.transform, sizeof(m44GPU));
    }
    for (const auto& renderable : widget_renderables) {
        memcpy((m44GPU*) buffer_model_mats.mapped_ptr + i++, &renderable.transform, sizeof(m44GPU));
    }

    struct CompositeData {
        m44GPU inverse_vp;
        v4 camera_position;
        
        m44GPU light_vp;
        v4 sun_data;
        v4 ambient;
        v4 rim_alpha_width_start;
        
        v2 clip_planes;
    } composite_data;
    composite_data.inverse_vp = (m44GPU) math::inverse(viewport.camera->vp);
    composite_data.camera_position = v4(viewport.camera->position, 1.0f);
    composite_data.clip_planes = v2(0.0f, 20.0f);
    composite_data.light_vp = sun_cam_data.vp;
    composite_data.sun_data = v4(sun_vec, scene_data.sun_intensity);
    composite_data.ambient = v4(scene_data.ambient);
    composite_data.rim_alpha_width_start = v4(scene_data.rim_alpha_width_start, 0.0f);

    auto [pubo_composite, fubo_composite] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&composite_data, 1));
    buffer_composite_data             = *pubo_composite;
}

void RenderScene::pre_render() {
    if (viewport.size.x < 2 || viewport.size.y < 2) {
        update_size(v2i(2, 2));
        console_error("RenderScene::pre_render without RenderScene::image call", "renderer", ErrorType_Warning);
    }
    viewport.pre_render();
}


void RenderScene::update() {
    submitted_lights.clear();
}

vuk::Future RenderScene::render(vuk::Allocator& frame_allocator, vuk::Future target) {
    ZoneScoped;
    
    if (cull_pause || user_pause) {
        auto rg = make_shared<vuk::RenderGraph>("graph");
        rg->attach_image("target_output", vuk::ImageAttachment::from_texture(render_target));
        return vuk::Future {rg, "target_output"};
    }

    for (auto it = emitters.begin(); it != emitters.end();) {
        if (it->deinstance_at <= (Input::time - Input::delta_time - 0.1f))
            it = emitters.erase(it);
        else
            it++;
    }
    std::erase_if(emitters, [](const EmitterGPU& emitter) {
        return emitter.deinstance_at <= (Input::time - Input::delta_time - 0.1f);
    });

    
    for (Renderable& renderable : renderables) {
        upload_dependencies(renderable);
    }
    for (Renderable& renderable : widget_renderables) {
        upload_dependencies(renderable);
    }
    for (EmitterGPU& emitter : emitters) {
        upload_dependencies(emitter);
    }

    game.renderer.wait_for_futures();
    
    console({.str = "render_scene render", .group = "render_scene", .frame_tags = {"render_scene"}});

    _upload_buffer_objects(frame_allocator);
    
    auto rg = make_shared<vuk::RenderGraph>("graph");
    rg->attach_in("target_input", std::move(target));
    // Set up the pass to draw the textured cube, with a color and a depth attachment
    // clang-format off
    rg->add_pass({
        .name = "emitter_update",
        .resources = {},
        .execute = [this](vuk::CommandBuffer& command_buffer) {
            for (auto& emitter : emitters) {
                update_emitter(emitter, command_buffer);
            }
        }
    });
    rg->add_pass({
        .name = "sun_depth",
        .resources = {
            "sun_depth_input"_image   >> vuk::eDepthStencilRW >> "sun_depth_output"
        },
        .execute = [this](vuk::CommandBuffer& command_buffer) {
            ZoneScoped;
            // Prepare render
            command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
                .set_viewport(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {2048, 2048}})
                .set_scissor(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {2048, 2048}})
                .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                          .depthTestEnable  = true,
                          .depthWriteEnable = true,
                          .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
                })
                .broadcast_color_blend({vuk::BlendPreset::eOff});
            
            command_buffer
                .bind_buffer(0, CAMERA_BINDING, buffer_sun_camera_data)
                .bind_buffer(0, MODEL_BINDING, buffer_model_mats);
            
            command_buffer
                .set_rasterization({.cullMode = vuk::CullModeFlagBits::eNone})
                .bind_graphics_pipeline("directional_depth");
            
            int item_index = 0;
            for (Renderable& renderable : renderables) {
                render_shadow(renderable, command_buffer, &item_index);
            }
        }
    });
    rg->add_pass({
        .name = "forward",
        .resources = {
            "base_color_input"_image >> vuk::eColorWrite     >> "base_color_output",
            "emissive_input"_image >> vuk::eColorWrite     >> "emissive_output",
            "normal_input"_image  >> vuk::eColorWrite     >> "normal_output",
            "info_input"_image    >> vuk::eColorWrite     >> "info_output",
            "depth_input"_image   >> vuk::eDepthStencilRW >> "depth_output"
        },
        .execute = [this](vuk::CommandBuffer& command_buffer) {
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
                .set_color_blend("base_color_input", vuk::BlendPreset::eAlphaBlend)
                .set_color_blend("emissive_input", vuk::BlendPreset::eAlphaBlend)
                .set_color_blend("normal_input", vuk::BlendPreset::eOff)
                .set_color_blend("info_input", vuk::BlendPreset::eOff);
            
            command_buffer
                .bind_buffer(0, CAMERA_BINDING, buffer_camera_data)
                .bind_buffer(0, MODEL_BINDING, buffer_model_mats);
            

            // Render items
            int item_index = 0;
            for (Renderable& renderable : renderables) {
                render_item(renderable, command_buffer, &item_index);
            }

            for (auto& emitter : emitters) {
                render_particles(emitter, command_buffer);
            }
            // Render grid
            if (render_grid) {
                auto grid_view = game.renderer.get_texture("textures/grid.sbtex")->value.view.get();
                command_buffer
                    .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                        .depthTestEnable  = true,
                        .depthWriteEnable = false,
                        .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
                    })
                    .bind_graphics_pipeline("grid_3d")
                    .set_rasterization({})
                    .bind_buffer(0, CAMERA_BINDING, buffer_camera_data)
                    .bind_image(0, 1, grid_view).bind_sampler(0, 1, Sampler().anisotropy(true).get())
                    .draw(6, 1, 0, 0);
            }
        }
    });
    rg->add_pass({
        .name = "widget",
        .resources = {
            "widget_input"_image >> vuk::eColorWrite     >> "widget_output",
            "widget_depth_input"_image   >> vuk::eDepthStencilRW >> "widget_depth_output"
        },
        .execute = [this](vuk::CommandBuffer& command_buffer) {
            ZoneScoped;
            if (render_widgets) {
                // Prepare render
                command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
                    .set_viewport(0, vuk::Rect2D::framebuffer())
                    .set_scissor(0, vuk::Rect2D::framebuffer())
                    .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                              .depthTestEnable  = true,
                              .depthWriteEnable = true,
                              .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
                    })
                    .broadcast_color_blend({vuk::BlendPreset::eAlphaBlend});
                
                command_buffer
                    .bind_buffer(0, CAMERA_BINDING, buffer_camera_data)
                    .bind_buffer(0, MODEL_BINDING, buffer_model_mats);
                // Render items
                int item_index = 0 + renderables.size();
                for (Renderable& renderable : widget_renderables) {
                    render_widget(renderable, command_buffer, &item_index);
                }
            }
        }});
    rg->add_pass(vuk::Pass {
        .name = "postprocess_apply",
        .resources = {
            "base_color_output"_image >> vuk::eComputeSampled,
            "emissive_output"_image >> vuk::eComputeSampled,
            "normal_output"_image  >> vuk::eComputeSampled,
            "depth_output"_image   >> vuk::eComputeSampled,
            "widget_output"_image >> vuk::eComputeSampled,
            "widget_depth_output"_image >> vuk::eComputeSampled,
            "sun_depth_output"_image >> vuk::eComputeSampled,
            "target_input"_image   >> vuk::eComputeWrite >> "target_output",
        },
        .execute =
            [this](vuk::CommandBuffer& cmd) {
                cmd.bind_compute_pipeline("postprocess");

                auto sampler = Sampler().filter(Filter_Nearest).get();
                cmd.bind_image(0, 0, "base_color_output").bind_sampler(0, 0, sampler);
                cmd.bind_image(0, 1, "emissive_output").bind_sampler(0, 1, sampler);
                cmd.bind_image(0, 2, "normal_output").bind_sampler(0, 2, sampler);
                cmd.bind_image(0, 3, "depth_output").bind_sampler(0, 3, sampler);
                cmd.bind_image(0, 4, "widget_output").bind_sampler(0, 4, sampler);
                cmd.bind_image(0, 5, "widget_depth_output").bind_sampler(0, 5, sampler);
                cmd.bind_image(0, 6, "sun_depth_output").bind_sampler(0, 6, sampler);
                cmd.bind_image(0, 7, "target_input");

                cmd.bind_buffer(0, 8, buffer_composite_data);
                
                auto target      = *cmd.get_resource_image_attachment("target_input");
                auto target_size = target.extent.extent;
                cmd.specialize_constants(0, target_size.width);
                cmd.specialize_constants(1, target_size.height);

                cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, post_process_data);

                cmd.dispatch_invocations(target_size.width, target_size.height);
            },
    });

    if (math::contains(range2i(v2i(0), v2i(viewport.size)), query)) {
        auto info_storage_buffer = **vuk::allocate_buffer(*game.renderer.global_allocator, { vuk::MemoryUsage::eGPUtoCPU, sizeof(u32), 1});
        rg->attach_buffer("info_storage", info_storage_buffer);
	    rg->add_pass({
            .name  = "read",
			.resources = { 
                "info_output"_image   >> vuk::eTransferRead,
                "info_storage"_buffer >> vuk::eMemoryWrite >> "info_readable"},
            .execute = [this](vuk::CommandBuffer& command_buffer) {
                command_buffer.copy_image_to_buffer("info_output", "info_storage", {
                    .imageSubresource = {.aspectMask = vuk::ImageAspectFlagBits::eColor},
                    .imageOffset = {query.x, query.y, 0},
                    .imageExtent = {1,1,1}
                });
				query = v2i(-1, -1);
            }
        });
        fut_query_result = vuk::Future{rg, "info_readable"};
    }
    // clang-format on

    auto clear_color       = vuk::ClearColor(scene_data.fog_color);
    auto clear_transparent = vuk::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    auto info_clear_color  = vuk::ClearColor {-1u, -1u, -1u, -1u};
    auto depth_clear_value = vuk::ClearDepthStencil{0.0f, 0};
    rg->attach_and_clear_image("base_color_input", {.format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1}, clear_color);
    rg->attach_and_clear_image("emissive_input", {.format = vuk::Format::eR16G16B16A16Sfloat}, clear_color);
    rg->attach_and_clear_image("normal_input", {.format = vuk::Format::eR16G16B16A16Sfloat}, clear_color);
    rg->attach_and_clear_image("depth_input", {.format = vuk::Format::eD32Sfloat}, depth_clear_value);
    rg->attach_and_clear_image("widget_input", {.format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1}, clear_transparent);
    rg->attach_and_clear_image("widget_depth_input", {.format = vuk::Format::eD32Sfloat}, depth_clear_value);
    rg->attach_and_clear_image("sun_depth_input", {.extent = {.extent = {2048, 2048, 1}}, .format = vuk::Format::eD16Unorm, .sample_count = vuk::Samples::e1}, depth_clear_value);
    rg->attach_and_clear_image("info_input", {.format = vuk::Format::eR32Uint}, info_clear_color);

    rg->inference_rule("base_color_input", vuk::same_extent_as("target_input"));
    rg->inference_rule("widget_input", vuk::same_extent_as("target_input"));
    
    return vuk::Future {rg, "target_output"};
}

void RenderScene::cleanup(vuk::Allocator& allocator) {
    game.renderer.scenes.remove_value(this);
}

void widget_setup() {
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    vuk::PipelineBaseCreateInfo pci;
    pci.add_glsl(get_contents("src/shaders/widget.vert"), "src/shaders/widget.vert");
    pci.add_glsl(get_contents("src/shaders/widget.frag"), "src/shaders/widget.frag");
    game.renderer.context->create_named_pipeline("widget", pci);
    
    MaterialCPU widget_mat = { .file_path = "widget", .shader_name = "widget" };
    upload_material(widget_mat);
}

Renderable& RenderScene::quick_mesh(const MeshCPU& mesh_cpu, bool frame_allocated, bool widget) {
    if (widget)
        widget_setup();
    
    Renderable r;
    r.mesh_asset_path = upload_mesh(mesh_cpu, frame_allocated);
    r.material_asset_path = widget ? "widget" : "default";
    r.frame_allocated = frame_allocated;
    
    return *(widget ? widget_renderables : renderables).emplace(r);
}

Renderable& RenderScene::quick_mesh(const string& mesh_name, bool frame_allocated, bool widget) {
    if (widget)
        widget_setup();
    
    Renderable r;
    r.mesh_asset_path = mesh_name;
    r.material_asset_path = widget ? "widget" : "default";
    r.frame_allocated = frame_allocated;
    
    return *(widget ? widget_renderables : renderables).emplace(r);
}

void material_setup() {
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    upload_mesh(generate_icosphere(3));
}

Renderable& RenderScene::quick_material(const MaterialCPU& material_cpu, bool frame_allocated) {
    material_setup();
    
    Renderable r;
    r.mesh_asset_path = "icosphere_subdivisions:3";
    r.material_asset_path = upload_material(material_cpu, frame_allocated);
    r.frame_allocated = frame_allocated;
    
    return *renderables.emplace(r);
}




}
