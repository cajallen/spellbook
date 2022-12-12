#pragma once

#include "general/vector.hpp"
#include "renderer/renderer.hpp"
#include "renderer/camera.hpp"

namespace spellbook {

struct FormattedVertex {
    v3 position;
    Color color;
    f32 width;
};

MeshCPU generate_cube(v3 center, v3 extents, Color vertex_color = palette::black);
MeshCPU generate_icosphere(int subdivisions);
MeshCPU generate_formatted_line(Camera* camera, vector<FormattedVertex> vertices);

}

