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

    struct TileState {
        TilePrefab prefab;
        u32 rotation;
    };
    
    umap<v3i, LizardPrefab> lizards;
    umap<v3i, TileState> tiles;
    umap<v3i, SpawnerPrefab> spawners;
    umap<v3i, ConsumerPrefab> consumers;

    void add_prefab(const LizardPrefab& prefab, v3i pos) { lizards[pos] = prefab; }
    void add_prefab(const TilePrefab& prefab, v3i pos, u32 rotation) { tiles[pos] = {prefab, rotation}; }
    void add_prefab(const SpawnerPrefab& prefab, v3i pos) { spawners[pos] = prefab; }
    void add_prefab(const ConsumerPrefab& prefab, v3i pos) { consumers[pos] = prefab; }
};
JSON_IMPL(MapPrefab::TileState, prefab, rotation);
JSON_IMPL(MapPrefab, lizards, tiles, spawners, consumers);

void inspect(MapPrefab*);
Scene* instance_map(const MapPrefab&, const string& name);


}
