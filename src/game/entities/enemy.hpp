#pragma once

#include <entt/entity/fwd.hpp>

#include "general/string.hpp"
#include "general/json.hpp"
#include "general/geometry.hpp"
#include "game/game_file.hpp"
#include "game/shop.hpp"
#include "game/entities/stat.hpp"

namespace spellbook {

struct Scene;

enum EnemyType {
    EnemyType_Empty
};

struct EnemyPrefab {
    EnemyType type = EnemyType_Empty;
    string model_path;
    string file_path;
    string hurt_path;
    
    float max_health = 0.0f;
    float max_speed = 0.0f;
    float scale = 0.5f;

    // just use the component directly lol
    DropChance drops;
};

struct Enemy {
    vector<v3i> pathing = {};
    Stat max_speed = {};

    std::pair<bool, v3i> position_target;
    std::pair<u64, entt::entity> taunt;
    id_ptr<Ability> ability;
};

JSON_IMPL(EnemyPrefab, type, model_path, hurt_path, max_health, max_speed, scale, drops);

entt::entity instance_prefab(Scene*, const EnemyPrefab&, v3i location);
bool inspect(EnemyPrefab*);
void travel_system(Scene* scene);


}