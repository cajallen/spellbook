#pragma once

#include "general/vector.hpp"
#include "renderer/renderer.hpp"
#include "renderer/camera.hpp"

namespace spellbook {

struct FormattedVertex {
    v3 position;
    Color color;
    f32 width;

    static constexpr FormattedVertex separate() {
        return {v3(0.0f), Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f};
    }
};

MeshCPU generate_cube(v3 center, v3 extents, Color vertex_color = palette::black);
MeshCPU generate_icosphere(int subdivisions);
MeshCPU generate_formatted_line(Camera* camera, vector<FormattedVertex> vertices);

void add_formatted_square(vector<FormattedVertex>& vertices, v3 center, v3 axis_1, v3 axis_2, Color color, float width);

}

