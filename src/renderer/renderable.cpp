#include "renderable.hpp"

#include <imgui.h>
#include <tracy/Tracy.hpp>

#include "extension/imgui_extra.hpp"
#include "game/game.hpp"
#include "editor/console.hpp"
#include "renderer/assets/skeleton.hpp"
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
    ZoneScoped;
    if (renderable.mesh_id == 0 || renderable.material_id == 0)
        return;
    game.renderer.get_mesh_or_upload(renderable.mesh_id);
    game.renderer.get_material_or_upload(renderable.material_id);
}

void render_item(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index) {
    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_id);
    MaterialGPU* material = game.renderer.get_material(renderable.material_id);
    assert_else (mesh != nullptr && material != nullptr) {
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

    // Draw call
    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, item_index ? (*item_index)++ : 0);
}

void render_widget(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index) {
    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_id);
    MaterialGPU* material = game.renderer.get_material(renderable.material_id);
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
    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, item_index ? (*item_index)++ : 0);
}

void render_shadow(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index) {
    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_id);
    if (mesh == nullptr) {
        (*item_index)++;
        return;
    }
    // Bind mesh
    command_buffer
        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32)
        .bind_buffer(0, BONES_BINDING, renderable.skeleton != nullptr ? renderable.skeleton->buffer.get() : SkeletonGPU::empty_buffer()->get());
                
    // Draw call
    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, (*item_index)++);
}


void render_item(StaticRenderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index) {
    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_id);
    MaterialGPU* material = game.renderer.get_material(renderable.material_id);
    assert_else (mesh != nullptr && material != nullptr) {
        if (item_index)
            (*item_index)++;
        return;
    }
    // Bind mesh
    command_buffer
        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32)
        .bind_buffer(0, BONES_BINDING, SkeletonGPU::empty_buffer()->get());

    // Bind Material
    command_buffer
        .set_rasterization({.cullMode = material->cull_mode})
        .bind_graphics_pipeline(material->pipeline);
    material->bind_parameters(command_buffer);
    material->bind_textures(command_buffer);

    // Draw call
    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, item_index ? (*item_index)++ : 0);
}

void render_shadow(StaticRenderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index) {
    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh_id);
    if (mesh == nullptr) {
        (*item_index)++;
        return;
    }
    // Bind mesh
    command_buffer
        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32)
        .bind_buffer(0, BONES_BINDING, SkeletonGPU::empty_buffer()->get());
                
    // Draw call
    command_buffer.draw_indexed(mesh->index_count, 1, 0, 0, (*item_index)++);
}

}
