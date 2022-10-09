#pragma once

#include "renderer.hpp"
#include "render_scene.hpp"
#include "vertex.hpp"
#include "math.hpp"

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

