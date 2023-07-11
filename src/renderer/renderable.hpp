#pragma once

#include <vuk/vuk_fwd.hpp>

#include "general/matrix.hpp"

namespace spellbook {

struct MeshGPU;
struct MaterialGPU;
struct SkeletonGPU;

struct StaticRenderable {
    u64 mesh_id;
    u64 material_id;
    m44GPU transform = m44GPU(m44::identity());
};

struct Renderable {
    u64 mesh_id;
    u64 material_id;
    m44GPU transform = m44GPU(m44::identity());

    SkeletonGPU* skeleton = nullptr;
    
    bool   frame_allocated     = false;
    u32    selection_id        = 0;
};

void inspect(Renderable* renderable);

void upload_dependencies(Renderable& renderable);

void render_item(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index);
void render_widget(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index);
void render_shadow(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index);
void render_item(StaticRenderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index);
void render_shadow(StaticRenderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index);

}
