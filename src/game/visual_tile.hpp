#pragma once

#include <array>

#include "general/color.hpp"
#include "general/string.hpp"
#include "general/umap.hpp"
#include "general/math/geometry.hpp"
#include "general/file/resource.hpp"

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
    uint8 yaw = 0;
    bool flip_x = false;
    bool flip_z = false;
};

struct VisualTileEntry {
    FilePath model_path = {};
    VisualTileRotation rotation = {};
};

struct VisualTileCorners {
    uint8 corners[8];

    VisualTileCorners() : corners{ 1,1,1,1,1,1,1,1 } {}
    
    bool operator==(const VisualTileCorners& oth) const {
        uint64 this_bits = uint64(*(const uint64*)corners);
        uint64 oth_bits = uint64(*(const uint64*)oth.corners);
        uint64 bitmask = this_bits & oth_bits;

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
    uint8& operator[](uint32 i) { return corners[i]; }
    const uint8& operator[](uint32 i) const { return corners[i]; }
};

struct VisualTilePrefab {
    FilePath model_path;
    VisualTileCorners corners;
};

struct VisualTileSet : Resource {
    vector<VisualTilePrefab> tiles;

    static constexpr string_view extension() { return ".sbjvts"; }
    static constexpr string_view dnd_key() { return "DND_VISUAL_TILE_SET"; }
    static FilePath folder() { return "visual_tile_sets"_resource; }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == VisualTileSet::extension(); }; }
};

// Component
struct VisualTileSetWidget {
    VisualTileSet* tile_set = nullptr;
    uint32 setting = 1;
};

inline VisualTileCorners from_jv_impl(const json_value& jv, VisualTileCorners* _) {
    json              j = from_jv<json>(jv);
    VisualTileCorners value;
    std::array<uint8, 8> corners;
    if (j.contains("corners"))
        corners = from_jv<std::array<uint8, 8>>(*j.at("corners"));
    memcpy(value.corners, corners.data(), 8);
    return value;
}
inline json_value to_jv(const VisualTileCorners& value) {
    auto j = json();
    std::array<uint8, 8> corners;
    memcpy(corners.data(), value.corners, 8);
    j["corners"] = make_shared<json_value>(to_jv(corners));
    return to_jv(j);
}
JSON_IMPL(VisualTilePrefab, corners, model_path);
JSON_IMPL(VisualTileSet, tiles);

VisualTileCorners apply_rotation(VisualTileCorners corners, VisualTileRotation rotation);
bool get_rotation(VisualTileCorners corners, VisualTileCorners target, VisualTileRotation& out_rotation, uint32 seed, bool flip_z = true);
umap<VisualTileCorners, vector<FilePath>> convert_to_entry_pool(const VisualTileSet& tile_set);
umap<v3i, VisualTileEntry> build_visual_tiles(umap<v3i, uint8>& solids, const umap<VisualTileCorners, vector<FilePath>>& entry_pool, v3i* single_tile = nullptr);

bool inspect(VisualTileSet* tile_set);

void visual_tile_widget_system(Scene* scene);

v3 to_vec(DirectionBits direction);
DirectionBits to_direction_bits(v3 v);
DirectionBits rotate_bits(uint32 direction, uint32 rotation);
DirectionBits rotate_bits(DirectionBits direction, uint32 rotation);

}

namespace std {
template <>
struct hash<spellbook::VisualTileCorners> {
    uint64 operator()(const spellbook::VisualTileCorners& value) const {
        uint64 data = uint64(*value.corners);
        return std::hash<uint64>()(data);
    }
};
}

