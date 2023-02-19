#pragma once

#include <vuk/vuk_fwd.hpp>

#include "general/matrix.hpp"

namespace spellbook {

struct MeshGPU;
struct MaterialGPU;
struct SkeletonGPU;

struct Renderable {
    string mesh_asset_path;
    string material_asset_path;
    m44GPU transform = (m44GPU) m44::identity();

    SkeletonGPU* skeleton = nullptr;
    
    bool   frame_allocated     = false;
    u32    selection_id        = 0;
};

void inspect(Renderable* renderable);

void upload_dependencies(Renderable& renderable);

void render_item(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index);
void render_widget(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index);
void render_shadow(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index);

}
