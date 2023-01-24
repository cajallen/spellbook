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
    
    umap<v3i, LizardPrefab> lizards;
    umap<v3i, TilePrefab> tiles;
    umap<v3i, SpawnerPrefab> spawners;
    umap<v3i, ConsumerPrefab> consumers;
    uset<v3i> solid_tiles;

    void add_prefab(const LizardPrefab& prefab, v3i pos) { lizards[pos] = prefab; }
    void add_prefab(const TilePrefab& prefab, v3i pos) { tiles[pos] = prefab; }
    void add_prefab(const SpawnerPrefab& prefab, v3i pos) { spawners[pos] = prefab; }
    void add_prefab(const ConsumerPrefab& prefab, v3i pos) { consumers[pos] = prefab; }
};
JSON_IMPL(MapPrefab, lizards, tiles, spawners, consumers, solid_tiles);

void inspect(MapPrefab*);
Scene* instance_map(const MapPrefab&, const string& name);


}
