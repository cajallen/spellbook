#pragma once

#include <entt/entity/entity.hpp>

#include "general/json.hpp"
#include "general/math/geometry.hpp"
#include "general/file_path.hpp"

namespace spellbook {

struct ConsumerPrefab {
    FilePath file_path;
    vector<FilePath> dependencies;
    
    FilePath shrine_model_path;
    FilePath egg_model_path;
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