#pragma once

#include <array>

#include "general/string.hpp"
#include "general/umap.hpp"
#include "general/geometry.hpp"

namespace spellbook {

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
    bool flip = false;
};

struct VisualTileEntry {
    string model_path = {};
    VisualTileRotation rotation = {};
};

struct VisualTilePrefab {
    std::array<u8, 8> corners;
    string model_path;
};

struct VisualTileSet {
    string file_path;
    vector<VisualTilePrefab> tiles;
};

JSON_IMPL(VisualTilePrefab, corners, model_path);
JSON_IMPL(VisualTileSet, tiles);

std::array<u8, 8> apply_rotation(std::array<u8, 8> corners, VisualTileRotation rotation);
bool get_rotation(std::array<u8, 8> corners, std::array<u8, 8> target, VisualTileRotation& out_rotation, u32 seed);
umap<std::array<u8, 8>, vector<string>> convert_to_entry_pool(const VisualTileSet& tile_set);
umap<v3i, VisualTileEntry> build_visual_tiles(const uset<v3i>& solids, const umap<std::array<u8, 8>, vector<string>>& entry_pool);

bool inspect(VisualTileSet* tile_set);

}

namespace std {
template <>
struct hash<std::array<u8, 8>> {
    u64 operator()(const std::array<u8, 8>& value) const {
        u64 data = u64(*value.data());
        return std::hash<u64>()(data);
    }
};
}
