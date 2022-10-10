#pragma once

#include "matrix.hpp"

namespace spellbook {

struct MeshGPU;
struct MaterialGPU;

struct Renderable {
    MeshGPU*     mesh         = nullptr;
    MaterialGPU* material     = nullptr;
    m44          transform    = {};
    u32          selection_id = 0;

    Renderable() = default;

    Renderable(MeshGPU& mesh, MaterialGPU& material, m44 transform = m44::identity())
        : mesh(&mesh), material(&material), transform(transform) {
    }
};

void inspect(Renderable* renderable);

}
