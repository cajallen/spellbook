#pragma once

#include "matrix.hpp"

namespace spellbook {

struct MeshGPU;
struct MaterialGPU;

struct Renderable {
    string mesh_asset_path     = {};
    string material_asset_path = {};
    m44    transform           = {};
    u32    selection_id        = 0;

    Renderable() = default;

    Renderable(string mesh_asset_path, string material_asset_path, m44 transform = m44::identity())
        : mesh_asset_path(std::move(mesh_asset_path)), material_asset_path(std::move(material_asset_path)), transform(transform) {
    }
};

void inspect(Renderable* renderable);

}
