#pragma once

#include <entt/fwd.hpp>

#include "string.hpp"
#include "json.hpp"
#include "geometry.hpp"

namespace spellbook {

struct ConsumerPrefab {
    string file_path;
    string model_path;
};

JSON_IMPL(ConsumerPrefab, model_path);

struct Scene;
entt::entity instance_prefab(Scene*, const ConsumerPrefab&, v3i location);

void inspect(ConsumerPrefab*);
void save_consumer(const ConsumerPrefab&);
ConsumerPrefab load_consumer(const string& input_path);
entt::entity instance_prefab(Scene* scene, const ConsumerPrefab& tile_prefab, v3i location);

}