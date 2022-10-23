#pragma once

#include "renderer.hpp"
#include "camera.hpp"

#include "vector.hpp"

namespace spellbook {

struct FormattedVertex {
    v3 position;
    Color color;
    f32 width;
};

MeshCPU generate_cube(v3 center, v3 extents);
MeshCPU generate_icosphere(int subdivisions);
MeshCPU generate_formatted_line(Camera* camera, vector<FormattedVertex> vertices);

}

