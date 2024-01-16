#pragma once

#include <entt/entity/entity.hpp>

#include "general/vector.hpp"
#include "general/umap.hpp"
#include "general/math/geometry.hpp"

namespace spellbook {

struct Scene;

struct MapTargeting {
    Scene* scene;

    umap<v3i, entt::entity> lizard_cache;
    uint32 lizard_frame_ack = 0;
    umap<float, umap<v3i, vector<entt::entity>>> enemy_cache;
    uint32 enemy_frame_ack = 0;

    vector<entt::entity> select_enemies(v3i pos, float in_future);
    entt::entity select_lizard(v3i pos, entt::entity exclude = {});
};

}