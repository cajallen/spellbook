#pragma once

#include <entt/fwd.hpp>

#include "general/umap.hpp"
#include "general/geometry.hpp"
#include "game/spawner.hpp"
#include "game/lizard.hpp"
#include "game/tile.hpp"
#include "game/enemy.hpp"
#include "game/consumer.hpp"
#include "game/game_file.hpp"

namespace spellbook {

struct MapPrefab {
    string file_path;

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

void inspect(MapPrefab*);
Scene* instance_map(const MapPrefab&, const string& name);


}
