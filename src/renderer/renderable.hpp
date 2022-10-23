#pragma once

#include "matrix.hpp"

namespace spellbook {

struct MeshGPU;
struct MaterialGPU;

struct Renderable {
    string mesh_asset_path     = "";
    string material_asset_path = "";
    m44    transform           = m44::identity();
    bool   frame_allocated     = false;
    u32    selection_id        = 0;
};

void inspect(Renderable* renderable);

}
