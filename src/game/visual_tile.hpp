#pragma once

#include <array>

#include "general/color.hpp"
#include "general/string.hpp"
#include "general/umap.hpp"
#include "general/geometry.hpp"

namespace spellbook {

struct Scene;

enum DirectionBits {
    NNN,
    NNP,
    NPN,
    NPP,
    PNN,
    PNP,
    PPN,
    PPP
};

constexpr v3i visual_direction_offsets[8] = {
    v3i(0,0,0), v3i(0,0,1), v3i(0,1,0), v3i(0,1,1),
    v3i(1,0,0), v3i(1,0,1), v3i(1,1,0), v3i(1,1,1)
};

struct VisualTileRotation {
    u8 yaw = 0;
    bool flip_x = false;
    bool flip_z = false;
};

struct VisualTileEntry {
    string model_path = {};
    VisualTileRotation rotation = {};
};

struct VisualTileCorners {
    std::array<u8, 8> corners;

    VisualTileCorners() { corners = { 1,1,1,1,1,1,1,1 }; }
    
    bool operator==(const VisualTileCorners& oth) const {
        u64 this_bits = u64(*(const u64*)corners.data());
        u64 oth_bits = u64(*(const u64*)oth.corners.data());
        u64 bitmask = this_bits & oth_bits;

        // Does each corner share a viable value?
        return
            bitmask & (0xffull << 8*7ull) &&
            bitmask & (0xffull << 8*6ull) &&
            bitmask & (0xffull << 8*5ull) &&
            bitmask & (0xffull << 8*4ull) &&
            bitmask & (0xffull << 8*3ull) &&
            bitmask & (0xffull << 8*2ull) &&
            bitmask & (0xffull << 8*1ull) &&
            bitmask & (0xffull << 8*0ull);
    }
    u8& operator[](u32 i) { return corners[i]; }
    const u8& operator[](u32 i) const { return corners[i]; }
};

struct VisualTilePrefab {
    string model_path;
    VisualTileCorners corners;
};

struct VisualTileSet {
    string file_path;
    vector<VisualTilePrefab> tiles;
};

// Component
struct VisualTileSetWidget {
    VisualTileSet* tile_set = nullptr;
    u32 setting = 1;
};

JSON_IMPL(VisualTileCorners, corners);
JSON_IMPL(VisualTilePrefab, corners, model_path);
JSON_IMPL(VisualTileSet, tiles);

VisualTileCorners apply_rotation(VisualTileCorners corners, VisualTileRotation rotation);
bool get_rotation(VisualTileCorners corners, VisualTileCorners target, VisualTileRotation& out_rotation, u32 seed);
umap<VisualTileCorners, vector<string>> convert_to_entry_pool(const VisualTileSet& tile_set);
umap<v3i, VisualTileEntry> build_visual_tiles(umap<v3i, u8>& solids, const umap<VisualTileCorners, vector<string>>& entry_pool, v3i* single_tile = nullptr);

bool inspect(VisualTileSet* tile_set);

void visual_tile_widget_system(Scene* scene);

}

namespace std {
template <>
struct hash<spellbook::VisualTileCorners> {
    u64 operator()(const spellbook::VisualTileCorners& value) const {
        u64 data = u64(*value.corners.data());
        return std::hash<u64>()(data);
    }
};
}

