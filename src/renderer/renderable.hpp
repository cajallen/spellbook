#pragma once

#include "matrix.hpp"

#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"

namespace spellbook {

struct Renderable {
    MeshGPU*     mesh         = {};
    MaterialGPU* material     = {};
    m44          transform    = {};
    u32          selection_id = 0;

    Renderable() = default;

    Renderable(MeshGPU& mesh, MaterialGPU& material, m44 transform = m44::identity())
        : mesh(&mesh), material(&material), transform(transform) {
    }
};

void inspect(Renderable* renderable);

}
