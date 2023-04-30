﻿#include "map_targeting.hpp"

#include "game/scene.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

vector<entt::entity> MapTargeting::select_enemies(v3i pos, float in_future) {
    if (enemy_frame_ack < scene->frame) {
        enemy_frame_ack = scene->frame;
        enemy_cache.clear();
    }
    
    float used_time = math::round((scene->time + in_future) * 20.0f) / 20.0f;
    if (enemy_cache.contains(used_time)) {
        auto& enemy_map = enemy_cache[used_time];
        return enemy_map.contains(pos) ? enemy_map[pos] : vector<entt::entity>{};
    }

    auto& enemy_map = enemy_cache.emplace(used_time, umap<v3i, vector<entt::entity>>{}).first->second;
    for (auto [entity, l_transform] : scene->registry.view<Enemy, LogicTransform>().each()) {
        v3i pos = math::round_cast(l_transform.position);
        auto traveler = scene->registry.try_get<Traveler>(entity);
        if (!traveler) {
            if (!enemy_map.contains(pos))
                enemy_map[pos] = {};

            enemy_map[pos].push_back(entity);
        } else {
            v3i expected_pos = math::round_cast(predict_pos(*traveler, l_transform.position, used_time - scene->time));
            
            if (!enemy_map.contains(expected_pos))
                enemy_map[expected_pos] = {};

            enemy_map[expected_pos].push_back(entity);
        }
    }

    return enemy_map[pos];
}

entt::entity MapTargeting::select_lizard(v3i pos) {
    if (lizard_frame_ack < scene->frame) {
        lizard_frame_ack = scene->frame;
        lizard_cache.clear();

        for (auto [entity, lizard, l_transform] : scene->registry.view<Lizard, LogicTransform>().each()) {
            lizard_cache[math::round_cast(l_transform.position)] = entity;
        }
    }
    
    if (!lizard_cache.contains(pos))
        return entt::null;
    return lizard_cache[pos];
}


}