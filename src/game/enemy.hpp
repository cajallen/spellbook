#pragma once

#include <entt/fwd.hpp>

#include "general/string.hpp"
#include "general/json.hpp"
#include "general/geometry.hpp"
#include "game/game_file.hpp"

namespace spellbook {

enum EnemyType {
    EnemyType_Empty
};

struct EnemyPrefab {
    EnemyType type = EnemyType_Empty;
    string model_path = "";
    string file_path = "";

    f32 max_health = 0.0f;
    f32 max_speed = 0.0f;
};

JSON_IMPL(EnemyPrefab, type, model_path, max_health, max_speed);

struct Scene;
entt::entity instance_prefab(Scene*, const EnemyPrefab&, v3i location);
bool inspect(EnemyPrefab*);


}