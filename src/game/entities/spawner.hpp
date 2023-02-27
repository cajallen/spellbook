#pragma once

#include <functional>
#include <entt/entity/fwd.hpp>

#include "general/vector.hpp"
#include "game/entities/stat.hpp"

namespace spellbook {

struct Scene;
struct Spawner;

struct RoundInfo {
    int round_number = 0;
    int wave_number = 0;
    int enemy_number = 0;

    void advance_round() {
        round_number++;
        wave_number = 0;
        enemy_number = 0;
    }
};

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
    int wave_count = 1;
};

struct Spawner {
    Stat delta_cost;
    std::function<bool(Spawner* spawner, float& cost_total)> wave_start;
    std::function<EnemySpawnInfo(Spawner* spawner, int)> select_enemy;
    EnemySpawnInfo selected_enemy;

    int round_ack = 0;
    bool wave_happening = false;
    float cost_total = 0.0f;
    float cooldown = 0.0f;
    
    RoundInfo* round_info;
};

JSON_IMPL(EnemySpawnInfo, prefab_path, cost, spawn_pre_delay, spawn_post_delay);
JSON_IMPL(SpawnerPrefab, enemy_selection, enemy_entries, wave_selection, wave_cost, delta_cost, wave_count);

bool inspect(EnemySpawnInfo* enemy_entry);
bool inspect(SpawnerPrefab* spawner_prefab);
entt::entity instance_prefab(Scene* scene, const SpawnerPrefab& spawner_prefab, v3i location);

void spawner_system(Scene* scene);

}