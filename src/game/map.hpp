#pragma once

#include "general/umap.hpp"
#include "general/geometry.hpp"
#include "game/game_file.hpp"

namespace spellbook {

struct Scene;

struct MapPrefab {
    string file_path;
    vector<string> dependencies;
    
    struct TileEntry {
        string prefab_path;
        u32 rotation;
    };
    umap<v3i, TileEntry> tiles;
    umap<v3i, string> spawners;
    umap<v3i, string> consumers;
    umap<v3i, string> lizards;
    umap<v3i, u8> solid_tiles;
};
JSON_IMPL(MapPrefab::TileEntry, prefab_path, rotation);
JSON_IMPL(MapPrefab, tiles, spawners, consumers, lizards, solid_tiles);

bool inspect(MapPrefab* prefab);
Scene* instance_map(const MapPrefab&, const string& name);


}
