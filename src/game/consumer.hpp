#pragma once

#include <entt/fwd.hpp>

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

void inspect(ConsumerPrefab*);
entt::entity instance_prefab(Scene* scene, const ConsumerPrefab& tile_prefab, v3i location);

}