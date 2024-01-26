#pragma once

#include <functional>
#include <entt/entity/fwd.hpp>

#include "general/vector.hpp"
#include "general/id_ptr.hpp"
#include "general/file/resource.hpp"
#include "game/entities/stat.hpp"

namespace spellbook {

struct Scene;
struct Spawner;

struct SpawnStateInfo {
    bool round_active = false;
    int round_number = -1;
    int wave_number = -1;
    int enemy_number = -1;

    void advance_round() {
        round_active = true;
        round_number++;
        wave_number = -1;
        enemy_number = -1;
    }
};

struct EnemySpawnInfo {
    FilePath enemy_prefab_path;
    float pre_delay;
    float post_delay;
};

struct WaveSpawnInfo {
    string name;
    vector<id_ptr<EnemySpawnInfo>> enemies;
    float pre_delay;
    float post_delay;
};

struct RoundSpawnInfo {
    string name;
    vector<id_ptr<WaveSpawnInfo>> waves;
};

struct LevelSpawnInfo {
    string name;
    vector<id_ptr<RoundSpawnInfo>> rounds;
};

struct SpawnerPrefab {
    FilePath file_path;
    vector<FilePath> dependencies;

    LevelSpawnInfo level_spawn_info;
    vector<id_ptr<RoundSpawnInfo>> rounds;
    vector<id_ptr<WaveSpawnInfo>> waves;
    vector<id_ptr<EnemySpawnInfo>> enemies;

    FilePath model_file_path;

    static constexpr string_view extension() { return ".sbjspw"; }
    static constexpr string_view dnd_key() { return "DND_SPAWNER"; }
    static FilePath folder() { return "spawners"_resource; }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == SpawnerPrefab::extension(); }; }
};

struct Spawner {
    LevelSpawnInfo spawn_info;
    SpawnStateInfo state;
    SpawnStateInfo* level_state;
    float cooldown = 0.0f;

    bool wave_spawning = false;
    bool enemy_spawning = false;
    bool spawn_wave = false;
    bool spawn_enemy = false;

    FilePath force_spawn_path;

    bool is_active();
};

JSON_IMPL(EnemySpawnInfo, enemy_prefab_path, pre_delay, post_delay);
JSON_IMPL(WaveSpawnInfo, name, enemies, pre_delay, post_delay);
JSON_IMPL(RoundSpawnInfo, name, waves);
JSON_IMPL(LevelSpawnInfo, name, rounds);
SpawnerPrefab from_jv_impl(const json_value& jv, SpawnerPrefab* _);
json_value to_jv(const SpawnerPrefab& value);

bool inspect(EnemySpawnInfo* enemy_spawn_info);
bool inspect(WaveSpawnInfo* wave_info);
bool inspect(RoundSpawnInfo* round_info);
bool inspect(LevelSpawnInfo* level_spawn_info);
bool inspect(SpawnerPrefab* spawner_prefab);
bool inspect(Spawner* spawner);
bool inspect(SpawnStateInfo* spawn_state_info);
entt::entity instance_prefab(Scene* scene, const SpawnerPrefab& spawner_prefab, v3i location);

void spawner_system(Scene* scene);

}
