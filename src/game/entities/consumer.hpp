#pragma once

#include <entt/entity/entity.hpp>

#include "general/string.hpp"
#include "general/json.hpp"
#include "general/geometry.hpp"
#include "game/game_file.hpp"

namespace spellbook {

struct ConsumerPrefab {
    string file_path;
    string shrine_model_path;
    string egg_model_path;
};

JSON_IMPL(ConsumerPrefab, shrine_model_path, egg_model_path);

struct Scene;
entt::entity instance_prefab(Scene*, const ConsumerPrefab&, v3i location);
bool inspect(ConsumerPrefab*);

void consumer_system(Scene* scene);

struct Shrine {
    entt::entity egg_entity;
    bool egg_attached;
};

struct Egg {};

}