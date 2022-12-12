#include "renderable.hpp"

#include <imgui.h>

#include "extension/imgui_extra.hpp"
#include "game/game.hpp"
#include "editor/console.hpp"
#include "renderer/assets/mesh_asset.hpp"

namespace spellbook {

void inspect(Renderable* renderable) {
    ImGui::PushID((void*) renderable);
    if (ImGui::TreeNode("Transform")) {
        ImGui::DragMat4("Transform", &renderable->transform, 0.01f, "%.3f");
        ImGui::TreePop();
    }
    ImGui::Separator();
    ImGui::PopID();
}

void upload_dependencies(Renderable& renderable) {
    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_asset_path);
    MaterialGPU* material = game.renderer.get_material(renderable.material_asset_path);

    if (renderable.mesh_asset_path.empty() || renderable.material_asset_path.empty())
        return;
    if (mesh == nullptr) {
        if (exists(to_resource_path(renderable.mesh_asset_path))) {
            game.renderer.upload_mesh(load_mesh(renderable.mesh_asset_path));
        } else {
            console({.str = "Renderable mesh asset not found: " + renderable.mesh_asset_path, .group = "assets", .frame_tags = {"render_scene"}});
        }
    }
    if (material == nullptr) {
        if (exists(to_resource_path(renderable.material_asset_path))) {
            game.renderer.upload_material(load_material(renderable.material_asset_path));
        } else {
            console({.str = "Renderable material asset not found: " + renderable.material_asset_path, .group = "assets", .frame_tags = {"render_scene"}});
        }
    }
}

void render_item(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index) {
    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_asset_path);
    MaterialGPU* material = game.renderer.get_material(renderable.material_asset_path);
    if (mesh == nullptr || material == nullptr) {
        if (item_index)
            (*item_index)++;
        return;
    }
    // Bind mesh
    command_buffer
        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32)
        .bind_buffer(0, BONES_BINDING, renderable.skeleton != nullptr ? renderable.skeleton->buffer.get() : SkeletonGPU::empty_buffer()->get());

    // Bind Material
    command_buffer
        .set_rasterization({.cullMode = material->cull_mode})
        .bind_graphics_pipeline(material->pipeline);
    material->bind_parameters(command_buffer);
    material->bind_textures(command_buffer);

    // Set ID for selection
    command_buffer.push_constants(vuk::ShaderStageFlagBits::eFragment, 0, renderable.selection_id);

    // Draw call
    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, item_index ? (*item_index)++ : 0);
}

void render_widget(Renderable& renderable, vuk::CommandBuffer& command_buffer) {
    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_asset_path);
    MaterialGPU* material = game.renderer.get_material(renderable.material_asset_path);
    assert_else(mesh != nullptr && material != nullptr)
        return;

    // Bind mesh
    command_buffer
        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_widget_format())
        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);

    // Bind Material
    command_buffer
        .set_rasterization({.cullMode = material->cull_mode})
        .bind_graphics_pipeline(material->pipeline);

    // Draw call
    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, 0);
}


}
