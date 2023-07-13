#pragma once

#include "game/visual_tile.hpp"
#include "general/math/geometry.hpp"
#include "general/vector.hpp"

namespace spellbook {

struct MeshCPU;

struct TileSetGeneratorSettings {
    vector<v3> mixed1_vertical_inside;
    vector<v3> mixed2_vertical_inside;
    vector<v3> type1_vertical_outside;
    vector<v3> type1_vertical_inside;
    vector<v3> type2_vertical_outside;
    vector<v3> type2_vertical_inside;

    vector<v3> mixed_side_vertical_1_inside;
    vector<v3> mixed_side_horizontal_1_inside;
    vector<v3> mixed_side_vertical_2_inside;
    vector<v3> mixed_side_horizontal_2_inside;
    vector<v3> type1_horizontal_outside;
    vector<v3> type1_horizontal_inside;
    vector<v3> type2_horizontal_outside;
    vector<v3> type2_horizontal_inside;
};

struct VisualTileMesh {
    string name;
    vector<v3> mesh1;
    vector<v3> mesh2;
    vector<v3> debug_mesh;
};
VisualTileMesh generate_visual_tile(const TileSetGeneratorSettings& settings, uint8 clear, uint8 type1, uint8 type2);
void generate_tile_set(const TileSetGeneratorSettings& settings);

void generate_example_set();

}
