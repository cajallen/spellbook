#include "render_scene.hpp"

#include <tracy/Tracy.hpp>
#include <functional>

#include <vuk/Partials.hpp>

#include "lib_ext/fmt_renderer.hpp"

#include "umap.hpp"
#include "file.hpp"
#include "samplers.hpp"
#include "matrix_math.hpp"
#include "game.hpp"
#include "viewport.hpp"

#include "renderable.hpp"
#include "assets/mesh_asset.hpp"

namespace spellbook {

void RenderScene::setup(vuk::Allocator& allocator) {
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("src/shaders/depth_outline.comp"), "depth_outline.comp");
        game.renderer.context->create_named_pipeline("postprocess", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci2;
        pci2.add_glsl(get_contents("src/shaders/infinite_plane.vert"), "infinite_plane.vert");
        pci2.add_glsl(get_contents("src/shaders/grid.frag"), "grid.frag");
        game.renderer.context->create_named_pipeline("grid_3d", pci2);
    }
}

slot<Renderable> RenderScene::add_renderable(Renderable renderable) {
    // console({.str = fmt_("Adding renderable: {}", renderable), .group = "renderables"});
    return renderables.add_element(std::move(renderable));
}
slot<Renderable> RenderScene::copy_renderable(slot<Renderable> renderable) {
    assert_else(renderables.valid(renderable)) return {};

    Renderable copy = renderables[renderable];
    // console({.str = fmt_("Copying renderable: {}", copy), .group = "renderables"});
    return renderables.add_element(std::move(copy));
}

void RenderScene::delete_renderable(slot<Renderable> renderable) {
    if (!renderables.valid(renderable))
        return;
    // console({.str = fmt_("Deleting renderable"), .group = "renderables"});
    renderables.remove_element(renderable);
}

void RenderScene::_upload_buffer_objects(vuk::Allocator& allocator) {
    struct CameraData {
        m44GPU view;
        m44GPU proj;
        v4     position;
    } cam_data;
    cam_data.view     = (m44GPU) viewport.camera->view;
    cam_data.proj     = (m44GPU) viewport.camera->proj;
    cam_data.position = v4(viewport.camera->position, 1.0f);

    auto [pubo_camera, fubo_camera] = vuk::create_buffer_cross_device(allocator, vuk::MemoryUsage::eCPUtoGPU, std::span(&cam_data, 1));
    buffer_camera_data              = *pubo_camera;

    struct SceneData {
        v4 ambient;
        v4 fog;
        v4 sun_data;
        v2 rim_intensity_start;
    } scene_data_gpu;
    scene_data_gpu.ambient             = scene_data.ambient;
    scene_data_gpu.fog                 = v4(scene_data.fog_color, scene_data.fog_depth);
    scene_data_gpu.rim_intensity_start = scene_data.rim_intensity_start;
    scene_data_gpu.sun_data            = v4(scene_data.sun_direction, scene_data.sun_intensity);

    auto [pubo_scene, fubo_scene] = vuk::create_buffer_cross_device(allocator, vuk::MemoryUsage::eCPUtoGPU, std::span(&scene_data_gpu, 1));
    buffer_scene_data             = *pubo_scene;

    buffer_model_mats = **vuk::allocate_buffer_cross_device(allocator, {vuk::MemoryUsage::eCPUtoGPU, sizeof(m44GPU) * renderables.size(), 1});
    int i = 0;
    for (const auto& renderable : renderables) {
        auto transform_gpu = m44GPU(renderable.transform);
        memcpy(reinterpret_cast<m44GPU*>(buffer_model_mats.mapped_ptr) + i++, &transform_gpu, sizeof(m44GPU));
    }
}

vuk::Future RenderScene::render(vuk::Allocator& frame_allocator, vuk::Future target) {
    ZoneScoped;

    auto upload_item = [](Renderable& renderable) {
        MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_asset_path);
        MaterialGPU* material = game.renderer.get_material(renderable.material_asset_path);

        if (mesh == nullptr) {
            if (file_exists(get_resource_path(renderable.mesh_asset_path))) {
                game.renderer.upload_mesh(load_mesh(renderable.mesh_asset_path));
            } else {
                console({.str = "Renderable mesh asset not found: " + renderable.mesh_asset_path, .group = "assets", .frame_tags = {"render_scene"}});
            }
        }
        if (material == nullptr) {
            if (file_exists(get_resource_path(renderable.material_asset_path))) {
                game.renderer.upload_material(load_material(renderable.material_asset_path));
            } else {
                console({.str = "Renderable material asset not found: " + renderable.material_asset_path, .group = "assets", .frame_tags = {"render_scene"}});
            }
        }
    };
    for (Renderable& renderable : renderables) {
        upload_item(renderable);
    }

    game.renderer.wait_for_futures();
    
    console({.str = "render_scene render", .group = "render_scene", .frame_tags = {"render_scene"}});

    _upload_buffer_objects(frame_allocator);

    auto rg = make_shared<vuk::RenderGraph>("graph");
    rg->attach_in("target_input", std::move(target));
    // Set up the pass to draw the textured cube, with a color and a depth attachment
    // clang-format off
    rg->add_pass({
        .name = "forward",
        .resources = {
            "forward_input"_image >> vuk::eColorWrite     >> "forward_output",
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
                .set_color_blend("forward_input", vuk::BlendPreset::eAlphaBlend)
                .set_color_blend("normal_input", vuk::BlendPreset::eOff)
                .set_color_blend("info_input", vuk::BlendPreset::eOff);

            // Bind buffers
            command_buffer
                .bind_buffer(0, 0, buffer_camera_data)
                .bind_buffer(0, 1, buffer_scene_data)
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

                // Set ID for selection
                command_buffer.push_constants(vuk::ShaderStageFlagBits::eFragment, 0, renderable.selection_id);

                // Draw call
                command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, i++);
            };

            // Render items
            for (Renderable& renderable : renderables) {
                render_item(renderable);
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
                .bind_buffer(0, 0, buffer_camera_data)
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

                cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, post_process_data);

                cmd.dispatch_invocations(target_size.width, target_size.height);
            },
    });

    if (math::contains(range2i(v2i(0), v2i(viewport.size)), query)) {
        auto info_storage_buffer = **vuk::allocate_buffer_cross_device(*game.renderer.global_allocator, { vuk::MemoryUsage::eGPUtoCPU, sizeof(u32), 1});
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

    auto clear_color      = vuk::ClearColor {scene_data.fog_color.r, scene_data.fog_color.g, scene_data.fog_color.b, 1.0f};
    auto info_clear_color = vuk::ClearColor {-1u, -1u, -1u, -1u};
    auto depth_clear_value = vuk::ClearDepthStencil{0.0f, 0};
    rg->attach_and_clear_image("forward_input", {.format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1}, clear_color);
    rg->attach_and_clear_image("normal_input", {.format = vuk::Format::eR16G16B16A16Sfloat}, clear_color);
    rg->attach_and_clear_image("info_input", {.format = vuk::Format::eR32Uint}, info_clear_color);
    rg->attach_and_clear_image("depth_input", {.format = vuk::Format::eD32Sfloat}, depth_clear_value);

    rg->inference_rule("forward_input", vuk::same_extent_as("target_input"));

    return vuk::Future {rg, "target_output"};
}

void RenderScene::cleanup(vuk::Allocator& allocator) {}

void inspect(RenderScene* scene) {
    ImGui::Text("Viewport");
    inspect(&scene->viewport);
    ImGui::ColorEdit4("Ambient", scene->scene_data.ambient.data);
    ImGui::DragFloat("Rim Start", &scene->scene_data.rim_intensity_start.y, 0.01f);
    ImGui::DragFloat("Rim Intensity", &scene->scene_data.rim_intensity_start.x, 0.01f);
    ImGui::DragFloat3("Sun Direction", scene->scene_data.sun_direction.data, 0.01f);
    ImGui::DragFloat("Sun Intensity", &scene->scene_data.sun_intensity, 0.01f);
    ImGui::DragFloat2("Outline(D)", &scene->post_process_data.outline.x, 0.1f);
    ImGui::DragFloat2("Outline(N)", &scene->post_process_data.outline.z, 0.002f);
}

}
