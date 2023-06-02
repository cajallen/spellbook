#include "map_targeting.hpp"

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
    for (auto [entity, enemy, logic_tfm] : scene->registry.view<Enemy, LogicTransform>().each()) {
        auto traveler = scene->registry.try_get<Traveler>(entity);
        v3i pos_min, pos_max;
        if (!traveler) {
            pos_min = math::round_cast(logic_tfm.position - v3(0.1f, 0.1f, 0.0f));
            pos_max = math::round_cast(logic_tfm.position + v3(0.1f, 0.1f, 0.0f));
        } else {
            v3 expected_pos = predict_pos(*traveler, logic_tfm.position, used_time - scene->time);
            pos_min = math::round_cast(expected_pos - v3(0.1f, 0.1f, 0.0f));
            pos_max = math::round_cast(expected_pos + v3(0.1f, 0.1f, 0.0f));
        }
        for (int x = pos_min.x; x <= pos_max.x; x++) {
            for (int y = pos_min.y; y <= pos_max.y; y++) {
                v3i map_pos = v3i(x, y, pos_min.z);
                if (!enemy_map.contains(map_pos))
                    enemy_map[map_pos] = {};
    
                enemy_map[map_pos].push_back(entity);
            }
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