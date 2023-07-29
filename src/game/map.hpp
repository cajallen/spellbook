#pragma once

#include "general/umap.hpp"
#include "general/math/geometry.hpp"
#include "general/file_path.hpp"

namespace spellbook {

struct Scene;

struct MapPrefab {
    FilePath file_path;
    vector<FilePath> dependencies;
    
    struct TileEntry {
        FilePath prefab_path;
        uint32 rotation;
    };
    umap<v3i, TileEntry> tiles;
    umap<v3i, FilePath> spawners;
    umap<v3i, FilePath> consumers;
    umap<v3i, FilePath> lizards;
    umap<v3i, uint8> solid_tiles;
};
JSON_IMPL(MapPrefab::TileEntry, prefab_path, rotation);
JSON_IMPL(MapPrefab, tiles, spawners, consumers, lizards, solid_tiles);

bool inspect(MapPrefab* prefab);
Scene* instance_map(const MapPrefab&, const string& name);


}
