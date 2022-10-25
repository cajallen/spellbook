#pragma once

#include <entt/fwd.hpp>

#include "string.hpp"
#include "json.hpp"
#include "geometry.hpp"

namespace spellbook {

enum EnemyType {
    EnemyType_Empty,
    EnemyType_Banana
};

struct EnemyPrefab {
    EnemyType type = EnemyType_Empty;
    string model_path = "";
    string file_path = "";

    f32 max_health = 0.0f;
    f32 max_speed = 0.0f;
};

JSON_IMPL(EnemyPrefab, type, model_path);

struct Scene;
entt::entity instance_prefab(Scene*, const EnemyPrefab&, v3i location);

void inspect(EnemyPrefab*);

}