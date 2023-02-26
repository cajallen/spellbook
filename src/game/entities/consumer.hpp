#pragma once

#include <entt/entity/entity.hpp>

#include "general/string.hpp"
#include "general/json.hpp"
#include "general/geometry.hpp"
#include "game/game_file.hpp"

namespace spellbook {

struct ConsumerPrefab {
    string file_path;
    string model_path;
};

JSON_IMPL(ConsumerPrefab, model_path);

struct Scene;
entt::entity instance_prefab(Scene*, const ConsumerPrefab&, v3i location);
bool inspect(ConsumerPrefab*);

struct Consumer {
    f32 consume_distance = 0.02f;
    int amount_consumed = 0;
};

}