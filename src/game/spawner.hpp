#pragma once

#include <functional>

#include "game/enemy.hpp"
#include "game/stat.hpp"
#include "general/vector.hpp"

namespace spellbook {

struct Scene;
struct Spawner;

struct EnemySpawnInfo {
    string prefab_path;
    float cost;
    float spawn_pre_delay;
    float spawn_post_delay;
};

struct SpawnerPrefab {
    string file_path;
    
    enum EnemySelection {
        EnemySelection_Simple
    };
    enum WaveSelection {
        WaveSelection_Simple
    };

    EnemySelection enemy_selection;
    vector<EnemySpawnInfo> enemy_entries;

    WaveSelection wave_selection;
    f32 wave_cost;
    
    f32 delta_cost;
};

struct Spawner {
    Stat delta_cost;
    std::function<bool(f32 cost_total)> wave_start;
    std::function<EnemySpawnInfo(int)> select_enemy;
    EnemySpawnInfo selected_enemy;
    int wave_spawned = 0;

    bool wave_happening = false;
    f32 cost_total = 0.0f;
    f32 cooldown = 0.0f;
};

JSON_IMPL(EnemySpawnInfo, prefab_path, cost, spawn_pre_delay, spawn_post_delay);
JSON_IMPL(SpawnerPrefab, enemy_selection, enemy_entries, wave_selection, wave_cost, delta_cost);

std::function<EnemySpawnInfo(int)> simple_select_enemy(EnemySpawnInfo enemy);
std::function<bool(f32 cost_total)> simple_wave(f32 threshold);

void spawner_system(Scene* scene);

bool inspect(EnemySpawnInfo* enemy_entry);
bool inspect(SpawnerPrefab* spawner_prefab);
entt::entity instance_prefab(Scene* scene, const SpawnerPrefab& spawner_prefab, v3i location);

}