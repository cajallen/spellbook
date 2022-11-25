#pragma once

#include <functional>

#include "game/enemy.hpp"
#include "game/stat.hpp"
#include "lib/vector.hpp"

namespace spellbook {

struct Scene;

std::function<EnemyPrefab*(f32* cost_left, f32* cooldown)> simple_select_enemy(EnemyPrefab* enemy_prefab, f32 cost, f32 cooldown);
std::function<bool(f32 cost_total)> simple_wave(f32 threshold);

struct SpawnerPrefab {
    string file_path;
    
    enum EnemySelection {
        EnemySelection_Simple
    };
    enum WaveSelection {
        WaveSelection_Simple
    };

    EnemySelection enemy_selection;
    string enemy_prefab_path;
    f32 enemy_cost;
    f32 enemy_cooldown;

    WaveSelection wave_selection;
    f32 wave_cost;
    
    f32 delta_cost;
};

struct Spawner {
    // Would be nice to move this out of a two state system into something arbitrary
    Stat delta_cost;
    std::function<EnemyPrefab*(f32* cost_left, f32* cooldown)> select_enemy;
    std::function<bool(f32 cost_total)> wave_start;

    bool wave_happening = false;
    f32 cost_total = 0.0f;
    f32 cooldown = 0.0f;
};

JSON_IMPL(SpawnerPrefab, enemy_selection, enemy_prefab_path, enemy_cost, enemy_cooldown, wave_selection, wave_cost, delta_cost);

void spawner_system(Scene* scene);

void inspect(SpawnerPrefab* spawner_prefab);
void save_spawner(const SpawnerPrefab&);
SpawnerPrefab load_spawner(const string& input_path);
entt::entity instance_prefab(Scene* scene, const SpawnerPrefab& spawner_prefab, v3i location);

}