#pragma once

#include <entt/fwd.hpp>

#include "general/umap.hpp"
#include "general/geometry.hpp"
#include "game/spawner.hpp"
#include "game/lizard.hpp"
#include "game/tile.hpp"
#include "game/enemy.hpp"
#include "game/consumer.hpp"

namespace spellbook {

struct MapPrefab {
    string file_path;
    
    umap<v3i, LizardPrefab> lizards;
    umap<v3i, TilePrefab> tiles;
    umap<v3i, SpawnerPrefab> spawners;
    umap<v3i, ConsumerPrefab> consumers;

    void add_prefab(const LizardPrefab& prefab, v3i pos) { lizards[pos] = prefab; }
    void add_prefab(const TilePrefab& prefab, v3i pos) { tiles[pos] = prefab; }
    void add_prefab(const SpawnerPrefab& prefab, v3i pos) { spawners[pos] = prefab; }
    void add_prefab(const ConsumerPrefab& prefab, v3i pos) { consumers[pos] = prefab; }
};
JSON_IMPL(MapPrefab, lizards, tiles, spawners, consumers);

void inspect(MapPrefab*);
void save_map(const MapPrefab&);
MapPrefab load_map(const string& input_path);
Scene* instance_map(const MapPrefab&, const string& name);


}
