#pragma once

#include "general/vector.hpp"
#include "renderer/renderer.hpp"
#include "renderer/camera.hpp"

#include "general/navigation_path.hpp"

namespace spellbook {

struct Bitmask3D;

struct FormattedVertex {
    v3 position;
    Color color;
    float width;

    static constexpr FormattedVertex separate() {
        return {v3(0.0f), Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f};
    }
};

MeshCPU generate_cube(v3 center, v3 extents, Color vertex_color = palette::black);
MeshCPU generate_icosphere(int subdivisions);
MeshCPU generate_formatted_line(Camera* camera, vector<FormattedVertex> vertices);
MeshCPU generate_formatted_dot(Camera* camera, FormattedVertex vertex);
MeshCPU generate_formatted_3d_bitmask(Camera* camera, const Bitmask3D& bitmask);
MeshCPU generate_outline(Camera* camera, const Bitmask3D& bitmask, const vector<v3i>& places, const Color& color, float thickness);

void add_formatted_square(vector<FormattedVertex>& vertices, v3 center, v3 axis_1, v3 axis_2, Color color, float width);

struct PaletteCreateInfo {
    range saturation_range = {0.1f, 0.8f};
    int saturation_shifts = 60;
    
    range value_range = {0.0f, 0.7f};
    int value_shifts = 120;

    int palettes = 1;
    int hue_shifts = 400;
    float hue_shift_per_y = 1.0f;
};

void generate_palette(const PaletteCreateInfo& info);

void draw_path(RenderScene& render_scene, Path& path, const v3& pos);

}

