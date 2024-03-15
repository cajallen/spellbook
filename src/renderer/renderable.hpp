#pragma once

#include <vuk/vuk_fwd.hpp>

#include "general/math/matrix.hpp"

namespace spellbook {

struct MeshGPU;
struct MaterialGPU;
struct SkeletonGPU;

struct StaticRenderable {
    uint64 mesh_id;
    uint64 material_id;
    m44GPU transform = m44GPU(m44::identity());
};

struct Renderable {
    uint64 mesh_id;
    uint64 material_id;
    m44GPU transform = m44GPU(m44::identity());

    SkeletonGPU* skeleton = nullptr;
    
    bool   frame_allocated     = false;
    uint32 selection_id        = 0;
    int32 sort_index = 0;

    bool operator<(const Renderable& rhs) const {
        return sort_index < rhs.sort_index;
    }
    bool operator>(const Renderable& rhs) const {
        return sort_index > rhs.sort_index;
    }
};

void inspect(Renderable* renderable);

void upload_dependencies(Renderable& renderable);

void render_widget(Renderable& renderable, vuk::CommandBuffer& command_buffer, int* item_index);
}
