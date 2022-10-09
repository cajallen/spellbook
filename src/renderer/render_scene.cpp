#include "render_scene.hpp"

#include "umap.hpp"
#include "file.hpp"
#include "samplers.hpp"
#include "matrix_math.hpp"
#include "game.hpp"
#include "viewport.hpp"

#include "lib_ext/fmt_renderer.hpp"

#include <tracy/Tracy.hpp>
#include <functional>

#include "vuk/Partials.hpp"

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

void RenderScene::_upload_dependencies() {
    for (auto& _cpu : mesh_dependencies) {
        game.renderer.upload_mesh(_cpu, true);
    }
    for (auto& _cpu : material_dependencies) {
        game.renderer.upload_material(_cpu, true);
    }
    for (auto& _cpu : texture_dependencies) {
        game.renderer.upload_texture(_cpu, true);
    }
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

    buffer_model_mats = **vuk::allocate_buffer_cross_device(
        allocator, {vuk::MemoryUsage::eCPUtoGPU, sizeof(m44GPU) * (renderables.size() + frame_renderables.size()), 1});
    int i = 0;
    for (auto& renderable : renderables) {
        m44GPU transform_gpu = (m44GPU) (renderable.transform);
        memcpy(reinterpret_cast<m44GPU*>(buffer_model_mats.mapped_ptr) + i++, &transform_gpu, sizeof(m44GPU));
    }
    for (auto& renderable : frame_renderables) {
        m44GPU transform_gpu = (m44GPU) (renderable.transform);
        memcpy(reinterpret_cast<m44GPU*>(buffer_model_mats.mapped_ptr) + i++, &transform_gpu, sizeof(m44GPU));
    }
}

vuk::Future RenderScene::render(vuk::Allocator& frame_allocator, vuk::Future target) {
    ZoneScoped;
    console({.str = "render_scene render", .group = "render_scene", .frame_tags = {"render_scene"}});

    _upload_dependencies();
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
                if (renderable.mesh == nullptr || renderable.material == nullptr) {
                    i++;
                    return;
                }
                // Bind mesh
                command_buffer
                    .bind_vertex_buffer(0, renderable.mesh->vertex_buffer.get(), 0, vuk::Packed {
                        vuk::Format::eR32G32B32Sfloat, // position
                        vuk::Format::eR32G32B32Sfloat, // normal
                        vuk::Format::eR32G32B32Sfloat, // tangent
                        vuk::Format::eR32G32B32Sfloat, // color
                        vuk::Format::eR32G32Sfloat     // uv
                    })
                    .bind_index_buffer(renderable.mesh->index_buffer.get(), vuk::IndexType::eUint32);

                // Bind Material
                command_buffer
                    .set_rasterization({.cullMode = renderable.material->cull_mode})
                    .bind_graphics_pipeline(renderable.material->pipeline);
                renderable.material->bind_parameters(command_buffer);
                renderable.material->bind_textures(command_buffer);

                // Set ID for selection
                command_buffer.push_constants(vuk::ShaderStageFlagBits::eFragment, 0, renderable.selection_id);

                // Draw call
                command_buffer.draw_indexed(renderable.mesh->index_count, 1, 0, 0, i++);
            };

            // Render items
            for (Renderable& renderable : renderables) {
                render_item(renderable);
            }
            for (Renderable& renderable : frame_renderables) {
                render_item(renderable);
            }
            // Render grid
            auto grid_view = game.renderer.find_texture("grid")->view.get();
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

            frame_renderables.clear();
            mesh_dependencies.clear();
            material_dependencies.clear();
            texture_dependencies.clear();
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

    if (query.x >= 0 && query.y >= 0) {
        auto info_storage_buffer = **vuk::allocate_buffer_cross_device(*game.renderer.global_allocator, { vuk::MemoryUsage::eGPUtoCPU, (size_t) 64, 1});
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
    auto info_clear_color = vuk::ClearColor {0.0f, 0.0f, 0.0f, 0.0f};
    rg->attach_and_clear_image(
        "forward_input", {.format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1}, clear_color);
    rg->attach_and_clear_image("normal_input", {.format = vuk::Format::eR16G16B16A16Sfloat}, clear_color);
    rg->attach_and_clear_image("info_input", {.format = vuk::Format::eR32Uint}, info_clear_color);
    rg->attach_and_clear_image("depth_input", {.format = vuk::Format::eD32Sfloat}, vuk::ClearDepthStencil {0.0f, 0});

    rg->inference_rule("forward_input", vuk::same_extent_as("target_input"));

    return vuk::Future {rg, "target_output"};
}

void RenderScene::cleanup(vuk::Allocator& allocator) {}

// on added item
// add pipelines to renderer
// add textures to renderer
// add materials to renderer
// add meshes to renderer
// add renderables to renderer

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
