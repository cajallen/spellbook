#include "render_scene.hpp"

#include <tracy/Tracy.hpp>

#include <vuk/vsl/Core.hpp>
#include <vuk/RenderGraph.hpp>

#include "extension/imgui_extra.hpp"
#include "extension/fmt.hpp"
#include "general/math/matrix_math.hpp"
#include "general/input.hpp"
#include "renderer/sampler.hpp"
#include "renderer/viewport.hpp"
#include "renderer/renderable.hpp"
#include "renderer/renderer.hpp"
#include "renderer/runtime.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/fmt_renderer.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "renderer/font_manager.hpp"
#include "renderer/assets/particles.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/texture.hpp"


namespace spellbook {

void RenderScene::setup(vuk::Allocator& allocator) {
    scene_data.ambient               = Color(palette::white, 0.15f);
    scene_data.fog_color             = palette::black;
    scene_data.fog_depth             = -1.0f;
    scene_data.rim_alpha_width_start = v3(0.1f, 0.1f, 0.75f);
    scene_data.sun_direction         = quat(0.2432103f, 0.3503661f, 0.0885213f, 0.9076734f);
    scene_data.sun_intensity         = 1.0f;
    scene_data.water_color1          = Color::hsv(175.0f, 0.8f, 0.70f).rgb;
    scene_data.water_color2          = Color::hsv(195.0f, 0.9f, 0.70f).rgb;
    scene_data.water_intensity       = 3.0f;
    scene_data.water_level           = -0.5f;

    MeshCPU cube = generate_cube(v3(0.0f, 0.0f, -3.5f), v3(10000.0f, 10000.0f, 1.0f));
    MaterialCPU background_mat = MaterialCPU{.color_tint = palette::black};
    background_mat.file_path = "black_mat"_symbolic;
    uint64 background_mat_name = upload_material(std::move(background_mat));
    quick_renderable(cube, background_mat_name, false);

    metallic_refl_iv = get_gpu_asset_cache().get_texture_or_upload("metallic_refl.sbatex"_distributed).value.ia.image_view;
}

void RenderScene::window(v2i size) {
    viewport.start = (v2i) ImGui::GetWindowPos() + (v2i) ImGui::GetCursorPos();
    update_size(math::max(size, v2i(2, 2)));
}

void RenderScene::settings_gui() {
    ImGui::Checkbox("Pause", &user_pause);
    if (ImGui::TreeNode("Lighting")) {
        ImGui::ColorEdit4("Ambient", scene_data.ambient.data);
        ImGui::DragFloat3("rim_alpha_width_start", scene_data.rim_alpha_width_start.data, 0.01f);
        ImGui::ColorEdit3("water_color1", scene_data.water_color1.data);
        ImGui::ColorEdit3("water_color2", scene_data.water_color2.data);
        ImGui::DragFloat("water_intensity", &scene_data.water_intensity);
        ImGui::DragFloat("water_level", &scene_data.water_intensity);

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
    bool changed = viewport.update_size(new_size);
    //if (changed)
    //    render_target = vuk::allocate_texture(*get_renderer().global_allocator, vuk::Format::eB8G8R8A8Unorm, vuk::Extent3D(new_size));
}

Renderable* RenderScene::add_renderable(const Renderable& renderable) {
    // console({.str = fmt_("Adding renderable: {}", renderable), .group = "renderables"});
    return &*renderables.emplace(renderable);
}

void RenderScene::delete_renderable(Renderable* renderable) {
    // console({.str = fmt_("Deleting renderable"), .group = "renderables"});
    renderables.erase(renderables.get_iterator(renderable));
}

void RenderScene::delete_renderable(StaticRenderable* renderable) {
    // console({.str = fmt_("Deleting renderable"), .group = "renderables"});
    static_renderables.erase(static_renderables.get_iterator(renderable));
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
    sun_cam_data.vp     = (m44GPU) (math::orthographic(v3(30.0f, 30.0f, 40.0f)) * math::look(sun_vec * 25.0f, -sun_vec, v3::Z));
    CameraData top_cam_data;
    top_cam_data.vp     = (m44GPU) (math::orthographic(v3(20.0f, 20.0f, 0.2f)) * math::look(v3(0.0f, 0.0f, 0.1f + scene_data.water_level), -v3::Z + v3(0.01f, 0.005f, 0.0f), v3::Y));

    auto [pubo_camera, fubo_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&cam_data, 1));
    buffer_camera_data              = *pubo_camera;
    
    auto [pubo_sun_camera, fubo_sun_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&sun_cam_data, 1));
    buffer_sun_camera_data              = *pubo_sun_camera;

    auto [pubo_top_camera, fubo_top_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&top_cam_data, 1));
    buffer_top_camera_data              = *pubo_top_camera;

    struct CompositeData {
        m44GPU inverse_vp;
        v4 camera_position;
        v4 camera_normal;

        m44GPU light_vp;
        m44GPU top_vp;
        v4 sun_data;
        v4 ambient;
        v4 rim_alpha_width_start;

        v4 water_color1;
        v4 water_color2;
        float water_intensity;
        float water_level;
    } composite_data;
    composite_data.inverse_vp = (m44GPU) math::inverse(viewport.camera->vp);
    composite_data.camera_position = v4(viewport.camera->position, 1.0f);
    composite_data.camera_normal = v4(math::euler2vector(viewport.camera->heading), 1.0f);
    composite_data.light_vp = sun_cam_data.vp;
    composite_data.top_vp = top_cam_data.vp;
    composite_data.sun_data = v4(sun_vec, scene_data.sun_intensity);
    composite_data.ambient = v4(scene_data.ambient);
    composite_data.rim_alpha_width_start = v4(scene_data.rim_alpha_width_start, 0.0f);
    composite_data.water_color1 = v4(srgb2linear(scene_data.water_color1), 1.0);
    composite_data.water_color2 = v4(srgb2linear(scene_data.water_color2), 1.0);
    composite_data.water_intensity = scene_data.water_intensity;
    composite_data.water_level = scene_data.water_level;

    auto [pubo_composite, fubo_composite] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&composite_data, 1));
    buffer_composite_data             = *pubo_composite;

    m44 ui_view = math::translate(v3(-1.0f, -1.0f, 0.0f)) * math::scale(v3(2.0f, 2.0f, 1.0f)) * math::scale(v3(1.0f / viewport.size.x, 1.0f / viewport.size.y, 1.0f));
    m44GPU ui_view_gpu = m44GPU(ui_view);
    auto [pubo_ui_view, fubo_ui_view] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&ui_view_gpu, 1));
    buffer_ui_view              = *pubo_ui_view;
}

void RenderScene::setup_renderables_for_passes(vuk::Allocator& allocator) {
    ZoneScoped;

    for (auto& [material, material_users] : renderables_built) {
        for (auto& [mesh, mesh_users] : material_users) {
            mesh_users.clear();
        }
    }
    for (auto& [material, material_users] : rigged_renderables_built) {
        for (auto& [mesh, mesh_users] : material_users) {
            mesh_users.clear();
        }
    }

    uint32 count = 0;

    for (auto& renderable : static_renderables) {
        assert_else(get_gpu_asset_cache().materials.contains(renderable.material_id))
            continue;
        assert_else(get_gpu_asset_cache().meshes.contains(renderable.mesh_id))
            continue;

        auto& mat_map = renderables_built.try_emplace(renderable.material_id).first->second;
        auto& mesh_list = mat_map.try_emplace(renderable.mesh_id).first->second;
        mesh_list.emplace_back(0, &renderable.transform);
        count++;
    }
    
    for (auto& renderable : renderables) {
        if (!get_gpu_asset_cache().materials.contains(renderable.material_id))
            continue;
        if (!get_gpu_asset_cache().meshes.contains(renderable.mesh_id))
            continue;
        
        if (renderable.skeleton == nullptr) {
            auto& mat_map = renderables_built.try_emplace(renderable.material_id).first->second;
            auto& mesh_list = mat_map.try_emplace(renderable.mesh_id).first->second;
            mesh_list.emplace_back(renderable.selection_id, &renderable.transform);
            count++;
        } else {
            auto& mat_map = rigged_renderables_built.try_emplace(renderable.material_id).first->second;
            auto& mesh_list = mat_map.try_emplace(renderable.mesh_id).first->second;
            mesh_list.emplace_back(renderable.selection_id, &renderable.transform, renderable.skeleton);
            count++;
        }
    }

    for (auto it = renderables_built.begin(); it != renderables_built.end();) {
        auto& [material, material_users] = *it;

        for (auto it2 = material_users.begin(); it2 != material_users.end();) {
            auto& [mesh, mesh_users] = *it2;
            if (mesh_users.empty())
                it2 = material_users.erase(it2);
            else
                it2++;
        }

        if (material_users.empty())
            it = renderables_built.erase(it);
        else
            it++;
    }

    for (auto it = rigged_renderables_built.begin(); it != rigged_renderables_built.end();) {
        auto& [material, material_users] = *it;

        for (auto it2 = material_users.begin(); it2 != material_users.end();) {
            auto& [mesh, mesh_users] = *it2;
            if (mesh_users.empty())
                it2 = material_users.erase(it2);
            else
                it2++;
        }
        if (material_users.empty())
            it = rigged_renderables_built.erase(it);
        else
            it++;
    }

    widget_model_start = count;
    uint32 model_buffer_size = sizeof(m44GPU) * (count + widget_renderables.size());
    uint32 id_buffer_size = sizeof(uint32) * count;
    
    buffer_model_mats = **vuk::allocate_buffer(allocator, {vuk::MemoryUsage::eCPUtoGPU, model_buffer_size, 1});
    buffer_ids = **vuk::allocate_buffer(allocator, {vuk::MemoryUsage::eCPUtoGPU, id_buffer_size, 1});
    int32 i = 0;
    for (const auto& [mat_hash, mat_map] : renderables_built) {
        for (const auto& [mesh_hash, mesh_list] : mat_map) {
            for (auto& [id, transform] : mesh_list) {
                memcpy((m44GPU*) buffer_model_mats.mapped_ptr + i, transform, sizeof(float) * 16);
                *((uint32*) buffer_ids.mapped_ptr + i) = id;
                i++;
            }
        }
    }
    for (const auto& [mat_hash, mat_map] : rigged_renderables_built) {
        for (const auto& [mesh_hash, mesh_list] : mat_map) {
            for (const auto& [id, transform, skeleton] : mesh_list) {
                memcpy((m44GPU*) buffer_model_mats.mapped_ptr + i, transform, sizeof(float) * 16);
                *((uint32*) buffer_ids.mapped_ptr + i) = id;
                i++;
            }
        }
    }
    
    for (const auto& renderable : widget_renderables) {
        memcpy((m44GPU*) buffer_model_mats.mapped_ptr + i++, &renderable.transform, sizeof(m44GPU));
    }


    // 3D TEXT
    vector<Renderable> sorted_ui_renderables;
    for (const Renderable& r : ui_renderables) {
        if (r.sort_index < 0)
            sorted_ui_renderables.insert_search(r);
    }

    uint32 text_model_buffer_size = sizeof(m44GPU) * sorted_ui_renderables.size();
    buffer_3d_text_model_mats = **vuk::allocate_buffer(allocator, {vuk::MemoryUsage::eCPUtoGPU, text_model_buffer_size, 1});
    i = 0;
    for (const auto& renderable : sorted_ui_renderables) {
        if (renderable.sort_index < 0)
            memcpy((m44GPU*) buffer_3d_text_model_mats.mapped_ptr + i++, &renderable.transform, sizeof(m44GPU));
    }

}


void RenderScene::pre_render() {
    if (viewport.size.x < 2 || viewport.size.y < 2) {
        update_size(v2i(2, 2));
        log_error("RenderScene::pre_render without RenderScene::image call", "renderer");
    }
    viewport.pre_render();

    prune_emitters();
    
    for (Renderable& renderable : renderables) {
        upload_dependencies(renderable);
    }
    for (Renderable& renderable : widget_renderables) {
        upload_dependencies(renderable);
    }
    for (EmitterGPU& emitter : emitters) {
        upload_dependencies(emitter);
    }
}


void RenderScene::update() {
    if (!query.futures.empty() && query.futures.front().node && *query.futures.front().poll() == vuk::Signal::Status::eHostAvailable) {
        auto result_buf = *query.futures.front().get(*get_renderer().global_allocator, get_renderer().compiler);
        query.result = *((uint32*) result_buf.mapped_ptr);
        query.futures.pop();
    }
}

void RenderScene::render() {
    ZoneScoped;

    _upload_buffer_objects(*get_renderer().frame_allocator);
    setup_renderables_for_passes(*get_renderer().frame_allocator);
    
    post_process_data.time = Input::time;

    vector<vuk::Value<vuk::Buffer>> emitter_buffers_input;
    vector<EmitterGPU*> vector_emitters;

    for (auto& emitter : emitters) {
        string emitter_name = fmt_("emitter_buffer:{}", vector_emitters.size());
        emitter_buffers_input.push_back(vuk::acquire_buf(vuk::Name(emitter_name), emitter.particles_buffer.get(), vuk::Access::eComputeRW));
        vector_emitters.push_back(&emitter);
    }
    auto emitter_buffers_array = vuk::declare_array("emitter_buffers", span(emitter_buffers_input));

    auto emitter_update_pass = vuk::make_pass("emitter_update", [this, vector_emitters](vuk::CommandBuffer& command_buffer, VUK_ARG(vuk::Buffer[], vuk::Access::eComputeRW) emitter_buffers) {
        for (uint32 i = 0; i < emitters.size(); i++) {
            auto& emitter = *vector_emitters[i];
            auto& emitter_buffer = emitter_buffers[i];

            struct PC {
                uint32 spawn_count = 0;
                float dt;
            } pc;
            pc.spawn_count = math::max((Input::time - emitter.next_spawn) / emitter.rate, 0.0f);
            emitter.next_spawn += emitter.rate * pc.spawn_count;
            if (!emitter.emitting)
                pc.spawn_count = 0;
                
            pc.dt = Input::delta_time;
            command_buffer
                .bind_compute_pipeline("emitter")
                .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, pc)
                .bind_buffer(0, 0, emitter_buffer);
            *command_buffer.scratch_buffer<EmitterSettings>(0, 1) = emitter.settings;
            command_buffer.dispatch(emitter.settings.max_particles); 
        }
    });
    
    emitter_update_pass(emitter_buffers_array);
    emitter_buffers_array.submit(*get_renderer().frame_allocator, get_renderer().compiler);

    auto sun_depth_pass = vuk::make_pass("sun_depth", [this](vuk::CommandBuffer& command_buffer, VUK_IA(vuk::eDepthStencilRW) sun_depth) {
        ZoneScoped;
        // Prepare render
        command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
            .set_viewport(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {4096, 4096}})
            .set_scissor(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {4096, 4096}})
            .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                .depthTestEnable  = true,
                .depthWriteEnable = true,
                .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
            })
            .broadcast_color_blend({vuk::BlendPreset::eOff});
                
        command_buffer
            .bind_buffer(0, BONES_BINDING, SkeletonGPU::empty_buffer()->get())
            .bind_buffer(0, CAMERA_BINDING, buffer_sun_camera_data)
            .bind_buffer(0, MODEL_BINDING, buffer_model_mats)
            .bind_buffer(0, ID_BINDING, buffer_ids);

        command_buffer
            .set_rasterization({.cullMode = vuk::CullModeFlagBits::eNone})
            .bind_graphics_pipeline("directional_depth");
    
        int item_index = 0;
        for (const auto& [mat_hash, mat_map] : renderables_built) {
            for (const auto& [mesh_hash, mesh_list] : mat_map) {
                MeshGPU* mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                command_buffer
                    .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                    .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
                command_buffer.draw_indexed(mesh->index_count, mesh_list.size(), 0, 0, item_index);
                item_index += mesh_list.size();
            }
        }
        for (const auto& [mat_hash, mat_map] : rigged_renderables_built) {
            for (const auto& [mesh_hash, mesh_list] : mat_map) {
                MeshGPU* mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                command_buffer
                    .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                    .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
                for (auto& [id, transform, skeleton] : mesh_list) {
                    command_buffer.bind_buffer(0, BONES_BINDING, skeleton->buffer.get());
                    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, item_index++);
                }
            }
        }
        return sun_depth;
    });

	auto sun_depth = vuk::declare_ia("sun_depth");
	sun_depth->format = vuk::Format::eD16Unorm;
    sun_depth->extent = {4096, 4096, 1};
	sun_depth = vuk::clear_image(std::move(sun_depth), vuk::ClearDepthStencil{ 0.0f, 0 });
    
    sun_depth = sun_depth_pass(std::move(sun_depth));


    auto top_depth_pass = vuk::make_pass("top_depth", [this](vuk::CommandBuffer& command_buffer, VUK_IA(vuk::eDepthStencilRW) top_depth) {
        ZoneScoped;
        // Prepare render
        command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
            .set_viewport(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {1024, 1024}})
            .set_scissor(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {1024, 1024}})
            .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                        .depthTestEnable  = true,
                        .depthWriteEnable = true,
                        .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
            })
            .broadcast_color_blend({vuk::BlendPreset::eOff});
                
        command_buffer
            .bind_buffer(0, BONES_BINDING, SkeletonGPU::empty_buffer()->get())
            .bind_buffer(0, CAMERA_BINDING, buffer_top_camera_data)
            .bind_buffer(0, MODEL_BINDING, buffer_model_mats)
            .bind_buffer(0, ID_BINDING, buffer_ids);

        command_buffer
            .set_rasterization({.cullMode = vuk::CullModeFlagBits::eNone})
            .set_conservative({.mode = vuk::ConservativeRasterizationMode::eOverestimate, .overestimationAmount = 0.75f})
            .bind_graphics_pipeline("directional_depth");
            
        int item_index = 0;
        for (const auto& [mat_hash, mat_map] : renderables_built) {
            for (const auto& [mesh_hash, mesh_list] : mat_map) {
                MeshGPU* mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                command_buffer
                    .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                    .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
                command_buffer.draw_indexed(mesh->index_count, mesh_list.size(), 0, 0, item_index);
                item_index += mesh_list.size();
            }
        }
        for (const auto& [mat_hash, mat_map] : rigged_renderables_built) {
            for (const auto& [mesh_hash, mesh_list] : mat_map) {
                MeshGPU* mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                command_buffer
                    .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                    .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
                for (auto& [id, transform, skeleton] : mesh_list) {
                    command_buffer.bind_buffer(0, BONES_BINDING, skeleton->buffer.get());
                    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, item_index++);
                }
            }
        }
        return top_depth;
    });

	auto top_depth = vuk::declare_ia("top_depth");
	top_depth->format = vuk::Format::eD16Unorm;
    top_depth->extent = {1024, 1024, 1};
	top_depth = vuk::clear_image(std::move(top_depth), vuk::ClearDepthStencil{ 0.0f, 0 });
    
    top_depth = top_depth_pass(std::move(top_depth));


    auto depth_to_buffer = vuk::make_pass("top_depth_transfer1", [](vuk::CommandBuffer& cmd, VUK_IA(vuk::eTransferRead) top_depth, VUK_BA(vuk::eTransferWrite) top_blurred_temp0) {
        cmd.copy_image_to_buffer(top_depth, top_blurred_temp0, {.imageSubresource = {.aspectMask = vuk::ImageAspectFlagBits::eDepth}, .imageExtent = {1024, 1024, 1}});
        return top_blurred_temp0;
    });
    auto buffer_to_color = vuk::make_pass("top_depth_transfer2", [](vuk::CommandBuffer& cmd, VUK_BA(vuk::eTransferRead) top_blurred_temp0, VUK_IA(vuk::eTransferWrite) top_blurred_temp1) {
        cmd.copy_buffer_to_image(top_blurred_temp0, top_blurred_temp1, {.imageSubresource = {.aspectMask = vuk::ImageAspectFlagBits::eColor}, .imageExtent = {1024, 1024, 1}});
        return top_blurred_temp1;
    });
    auto top_depth_blur = vuk::make_pass("top_depth_blur", [](vuk::CommandBuffer& cmd, VUK_ARG(int32, vuk::eNone) axis, VUK_IA(vuk::eComputeRead) top_blurred_temp1, VUK_IA(vuk::eComputeWrite) top_blurred_temp2) {
        ZoneScoped;
        cmd.bind_compute_pipeline("blur");

        cmd.bind_image(0, axis, top_blurred_temp1);
        cmd.bind_image(0, axis, top_blurred_temp2);

        auto target_size = top_blurred_temp1->extent;

        cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, axis);
        cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 1 * sizeof(int32), target_size.width);
        cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 2 * sizeof(int32), target_size.height);

        cmd.dispatch_invocations(target_size.width, target_size.height);

        return top_blurred_temp2;
    });

    auto top_depth_temp_buf = vuk::declare_buf("top_depth_temp_buf", {.size = sizeof(uint16) * 1024 * 1024, .memory_usage = vuk::MemoryUsage::eGPUonly});
    auto top_depth_temp = vuk::declare_ia("top_depth_temp");
    top_depth_temp.same_shape_as(top_depth);

    top_depth_temp_buf = depth_to_buffer(top_depth, top_depth_temp_buf);
    top_depth = buffer_to_color(top_depth_temp_buf, top_depth);
    top_depth = top_depth_blur(vuk::make_constant(vuk::Name("axis:0"), 0), top_depth, top_depth_temp);
    top_depth = top_depth_blur(vuk::make_constant(vuk::Name("axis:1"), 1), top_depth, top_depth_temp);

    auto forward_pass = vuk::make_pass("forward", [this](
            vuk::CommandBuffer& command_buffer,
            VUK_IA(vuk::eColorWrite) base_color, 
            VUK_IA(vuk::eColorWrite) emissive,
            VUK_IA(vuk::eColorWrite) normal,
            VUK_IA(vuk::eColorWrite) info,
            VUK_IA(vuk::eDepthStencilRW) depth) {
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
            .set_color_blend(base_color, vuk::BlendPreset::eAlphaBlend)
            .set_color_blend(emissive, vuk::BlendPreset::eOff)
            .set_color_blend(normal, vuk::BlendPreset::eOff)
            .set_color_blend(info, vuk::BlendPreset::eOff);
            
        command_buffer
            .bind_buffer(0, BONES_BINDING, SkeletonGPU::empty_buffer()->get())
            .bind_buffer(0, CAMERA_BINDING, buffer_camera_data)
            .bind_buffer(0, MODEL_BINDING, buffer_model_mats)
            .bind_buffer(0, ID_BINDING, buffer_ids);

        int item_index = 0;
        for (const auto& [mat_hash, mat_map] : renderables_built) {
            MaterialGPU* material = get_gpu_asset_cache().materials.contains(mat_hash) ? &get_gpu_asset_cache().materials[mat_hash] : nullptr;
            assert_else(material->pipeline != nullptr);
            command_buffer
                .set_rasterization({.cullMode = material->cull_mode})
                .bind_graphics_pipeline(material->pipeline);
            material->bind_parameters(command_buffer);
            material->bind_textures(command_buffer);
            for (const auto& [mesh_hash, mesh_list] : mat_map) {
                MeshGPU* mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                command_buffer
                    .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                    .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
                command_buffer.draw_indexed(mesh->index_count, mesh_list.size(), 0, 0, item_index);
                item_index += mesh_list.size();
            }
        }
        for (const auto& [mat_hash, mat_map] : rigged_renderables_built) {
            MaterialGPU* material = get_gpu_asset_cache().materials.contains(mat_hash) ? &get_gpu_asset_cache().materials[mat_hash] : nullptr;
            command_buffer
                .set_rasterization({.cullMode = material->cull_mode})
                .bind_graphics_pipeline(material->pipeline);
            material->bind_parameters(command_buffer);
            material->bind_textures(command_buffer);
            for (const auto& [mesh_hash, mesh_list] : mat_map) {
                MeshGPU* mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                command_buffer
                    .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                    .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
                for (auto& [id, transform, skeleton] : mesh_list) {
                    command_buffer.bind_buffer(0, BONES_BINDING, skeleton->buffer.get());
                    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, item_index++);
                }
            }
        }            
            
        for (auto& emitter : emitters) {
            render_particles(emitter, command_buffer);
        }
        // Render grid
        if (render_grid) {
            auto grid_view = get_gpu_asset_cache().get_texture_or_upload("grid"_symbolic).value.ia.image_view;
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
        return std::make_tuple(base_color, emissive, normal, info, depth);
    });

    auto base_color_input = vuk::clear_image(vuk::declare_ia("base_color", {.extent = vuk::Extent3D(viewport.size), .format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1} ), vuk::ClearColor(scene_data.fog_color));
    auto emissive_input = vuk::clear_image(vuk::declare_ia("emissive_input", {.extent = vuk::Extent3D(viewport.size), .format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1} ), vuk::Transparent<float>);
    auto normal_input = vuk::clear_image(vuk::declare_ia("normal_input", {.extent = vuk::Extent3D(viewport.size), .format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1} ), vuk::Transparent<float>);
    auto info_input = vuk::clear_image(vuk::declare_ia("info_input", {.extent = vuk::Extent3D(viewport.size), .format = vuk::Format::eR32Uint, .sample_count = vuk::Samples::e1} ), vuk::ClearColor {-1u, -1u, -1u, -1u});
    auto depth_input = vuk::clear_image(vuk::declare_ia("depth_input", {.extent = vuk::Extent3D(viewport.size), .format = vuk::Format::eD32Sfloat, .sample_count = vuk::Samples::e1} ), vuk::ClearDepthStencil{0.0f, 0});

    auto [base_color, emissive, normal, info, depth] = forward_pass(base_color_input, emissive_input, normal_input, info_input, depth_input);


    auto widget_pass = vuk::make_pass("widget", [this](vuk::CommandBuffer& command_buffer, VUK_IA(vuk::eColorWrite) widget_color, VUK_IA(vuk::eDepthStencilRW) widget_depth) {
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
            int item_index = widget_model_start;
            for (Renderable& renderable : widget_renderables) {
                render_widget(renderable, command_buffer, &item_index);
            }
        }
        return std::make_tuple(widget_color, widget_depth);
    });

    auto widget_color_input = vuk::clear_image(vuk::declare_ia("widget_input", {.extent = vuk::Extent3D(viewport.size), .format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1} ), vuk::ClearColor(scene_data.fog_color));
    auto widget_depth_input = vuk::clear_image(vuk::declare_ia("widget_depth_input", {.extent = vuk::Extent3D(viewport.size), .format = vuk::Format::eD32Sfloat, .sample_count = vuk::Samples::e1} ), vuk::ClearDepthStencil{0.0f, 0});

    auto [widget_color, widget_depth] = widget_pass(widget_color_input, widget_depth_input);


    auto postprocess_pass = vuk::make_pass("post_process", [this](
            vuk::CommandBuffer& command_buffer, 
            VUK_IA(vuk::eComputeSampled) base_color,
            VUK_IA(vuk::eComputeSampled) emissive,
            VUK_IA(vuk::eComputeSampled) normal,
            VUK_IA(vuk::eComputeSampled) depth,
            VUK_IA(vuk::eComputeSampled) widget,
            VUK_IA(vuk::eComputeSampled) widget_depth,
            VUK_IA(vuk::eComputeSampled) sun_depth,
            VUK_IA(vuk::eComputeSampled) top_blurred,
            VUK_IA(vuk::eComputeWrite) target) {
        ZoneScoped;
        command_buffer.bind_compute_pipeline("postprocess");

        auto sampler = Sampler().filter(Filter_Linear).get();
        auto linear_clamp_sampler = Sampler().filter(Filter_Linear).address(Address_Clamp).get();
        auto sun_sampler = Sampler().filter(Filter_Nearest).address(Address_Border).get();
        sun_sampler.borderColor = vuk::BorderColor::eFloatTransparentBlack;

        command_buffer.bind_image(0, 0, base_color).bind_sampler(0, 0, sampler);
        command_buffer.bind_image(0, 1, emissive).bind_sampler(0, 1, sampler);
        command_buffer.bind_image(0, 2, normal).bind_sampler(0, 2, sampler);
        command_buffer.bind_image(0, 3, depth).bind_sampler(0, 3, sampler);
        command_buffer.bind_image(0, 4, widget).bind_sampler(0, 4, sampler);
        command_buffer.bind_image(0, 5, widget_depth).bind_sampler(0, 5, sampler);
        command_buffer.bind_image(0, 6, sun_depth).bind_sampler(0, 6, sun_sampler);
        command_buffer.bind_image(0, 7, top_blurred).bind_sampler(0, 7, linear_clamp_sampler);
        command_buffer.bind_image(0, 10, metallic_refl_iv).bind_sampler(0, 10, linear_clamp_sampler);
        command_buffer.bind_image(0, 8, target);

        command_buffer.bind_buffer(0, 9, buffer_composite_data);
            
        auto target_size = target->extent;
        command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, post_process_data);
        command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, sizeof(PostProcessData), target_size.width);
        command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, sizeof(PostProcessData) + sizeof(int32), target_size.height);

        command_buffer.dispatch_invocations(target_size.width, target_size.height);

        return target;
    });

    auto target = vuk::declare_ia("target", {.extent = vuk::Extent3D(viewport.size), .format = vuk::Format::eR16G16B16A16Sfloat});
    auto result = postprocess_pass(base_color, emissive, normal, depth, widget_color, widget_depth, sun_depth, top_depth, target);


    auto ui_pass = vuk::make_pass("spellbook_ui", [this](vuk::CommandBuffer& command_buffer, VUK_IA(vuk::eColorWrite) target) {
        ZoneScoped;

        command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
            .set_viewport(0, vuk::Rect2D::framebuffer())
            .set_scissor(0, vuk::Rect2D::framebuffer())
            .broadcast_color_blend({vuk::BlendPreset::eAlphaBlend})
            .set_color_blend(target, vuk::BlendPreset::eAlphaBlend);

        command_buffer
            .set_rasterization({.cullMode = vuk::CullModeFlagBits::eNone})
            .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                .depthTestEnable  = false,
                .depthWriteEnable = false,
                .depthCompareOp   = vuk::CompareOp::eNever,
            });

        vector<Renderable> sorted_ui_renderables;

        for (const Renderable& r : ui_renderables) {
            sorted_ui_renderables.insert_search(r);
        }

        command_buffer.bind_buffer(0, CAMERA_BINDING, buffer_camera_data).bind_buffer(0, MODEL_BINDING, buffer_3d_text_model_mats);
        bool in_positives = false;

        uint64 current_mat = 0;
        uint32 item_index = 0;
        for (const auto& renderable : sorted_ui_renderables) {
            if (!in_positives && renderable.sort_index >= 0) {
                in_positives = true;
                command_buffer.bind_buffer(0, VIEW_BINDING, buffer_ui_view);
            }
            if (current_mat != renderable.material_id) {
                MaterialGPU& mat = get_gpu_asset_cache().materials.at(renderable.material_id);
                command_buffer.bind_graphics_pipeline(mat.pipeline);
                mat.bind_parameters(command_buffer);
                mat.bind_textures(command_buffer);
                current_mat = renderable.material_id;
                uint32 single_channel = mat.extra_material_data.size() != 1 ? 0 : mat.extra_material_data.front();
                command_buffer.push_constants(vuk::ShaderStageFlagBits::eFragment, 0, single_channel);
            }

            MeshGPU& mesh = get_gpu_asset_cache().meshes.at(renderable.mesh_id);
            command_buffer
                .bind_vertex_buffer(0, mesh.vertex_buffer.get(), 0, VertexUI::get_format())
                .bind_index_buffer(mesh.index_buffer.get(), vuk::IndexType::eUint32);
            command_buffer.draw_indexed(mesh.index_count, 1, 0, 0, item_index++);
        }
        return target;
    });

    target = ui_pass(target);


    if (math::contains(range2i(v2i(0), v2i(viewport.size)), query.input)) {
        auto info_read_pass = vuk::make_pass("id_readback", [this](vuk::CommandBuffer& command_buffer, VUK_IA(vuk::eTransferRead) info, VUK_BA(vuk::eMemoryWrite) readback) {
            command_buffer.copy_image_to_buffer(info, readback, {
                .imageSubresource = {.aspectMask = vuk::ImageAspectFlagBits::eColor},
                .imageOffset = {query.input.x, query.input.y, 0},
                .imageExtent = {1,1,1}
            });
            query.input = v2i(-1, -1);

            return readback;
        });
        auto readback_buffer = vuk::declare_buf("info_readback", { .size = sizeof(uint32) * 1, .memory_usage = vuk::MemoryUsage::eGPUtoCPU });
        readback_buffer = info_read_pass(info, readback_buffer);
        readback_buffer.submit(*get_renderer().frame_allocator, get_renderer().compiler);
        query.futures.push(std::move(readback_buffer));
    }

    ImGui::Image(get_renderer().imgui_data.add_image(std::move(target)), ImVec2(viewport.size));

}

void RenderScene::prune_emitters() {
    for (auto it = emitters.begin(); it != emitters.end();) {
        if (it->deinstance_at <= (Input::time - Input::delta_time - 0.1f))
            it = emitters.erase(it);
        else
            it++;
    }
    std::erase_if(emitters, [](const EmitterGPU& emitter) {
        return emitter.deinstance_at <= (Input::time - Input::delta_time - 0.1f);
    });
}

void RenderScene::cleanup(vuk::Allocator& allocator) {
    emitters.clear();
    get_renderer().scenes.remove_value(this);
}

void widget_setup() {
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    vuk::PipelineBaseCreateInfo pci;
    pci.add_glsl(get_contents("shaders/widget.vert"_distributed), "shaders/widget.vert"_distributed.abs_string());
    pci.add_glsl(get_contents("shaders/widget.frag"_distributed), "shaders/widget.frag"_distributed.abs_string());
    get_renderer().runtime->runtime->create_named_pipeline("widget", pci);


    MaterialCPU widget_mat = { .shader_name = "widget" };
    widget_mat.file_path = "widget"_symbolic;
    upload_material(std::move(widget_mat));
}

Renderable& RenderScene::quick_mesh(const MeshCPU& mesh_cpu, bool frame_allocated, bool widget) {
    if (widget)
        widget_setup();
    
    Renderable r;
    r.mesh_id = upload_mesh(mesh_cpu, frame_allocated);
    r.material_id = hash_path(widget ? "widget"_symbolic : "default"_symbolic);
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
    r.mesh_id = hash_path("icosphere_subdivisions:3"_symbolic);
    r.material_id = upload_material(material_cpu, frame_allocated);
    r.frame_allocated = frame_allocated;
    
    return *renderables.emplace(r);
}

Renderable& RenderScene::quick_renderable(uint64 mesh_id, uint64 mat_id, bool frame_allocated) {
    material_setup();
    
    Renderable r;
    r.mesh_id = mesh_id;
    r.material_id = mat_id;
    r.frame_allocated = frame_allocated;
    
    return *renderables.emplace(r);
}

Renderable& RenderScene::quick_renderable(const MeshCPU& mesh, uint64 mat_id, bool frame_allocated) {
    material_setup();
    
    Renderable r;
    r.mesh_id = upload_mesh(mesh);
    r.material_id = mat_id;
    r.frame_allocated = frame_allocated;
    
    return *renderables.emplace(r);
}

Renderable& RenderScene::quick_renderable(uint64 mesh_id, const MaterialCPU& mat, bool frame_allocated) {
    material_setup();
    
    Renderable r;
    r.mesh_id = mesh_id;
    r.material_id = upload_material(mat);
    r.frame_allocated = frame_allocated;
    
    return *renderables.emplace(r);
}




}
