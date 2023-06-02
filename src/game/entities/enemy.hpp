﻿#pragma once

#include <entt/entity/fwd.hpp>

#include "general/string.hpp"
#include "general/json.hpp"
#include "general/geometry.hpp"
#include "game/game_file.hpp"
#include "game/shop.hpp"
#include "game/entities/stat.hpp"
#include "general/quaternion.hpp"

namespace spellbook {

struct Scene;

enum EnemyType {
    EnemyType_Empty
};

struct EnemyPrefab {
    EnemyType type = EnemyType_Empty;
    string base_model_path = "models/enemy_spider/spider_base.sbmod";
    string attachment_model_path;
    string file_path;
    string hurt_path;
    
    float max_health = 0.0f;
    float max_speed = 0.0f;
    float scale = 0.5f;

    // just use the component directly lol
    DropChance drops;
};

struct Traveler {
    struct Target {
        u32 priority;
        v3i pos;
    };
    
    Stat max_speed;

    Target target = {};
    vector<v3> pathing = {};
    
    void set_target(Target new_target) {
        assert_else(new_target.priority != 0);
        if (target.priority >= new_target.priority)
            return;
        target = new_target;
    }
    void reset_target() {
        target = {0, v3i{}};
    }
    bool has_target() const {
        return target.priority != 0u;
    }
};

struct Enemy {
    entt::entity attachment;
    entt::entity target_consumer;
};

struct Attachment {
    entt::entity base;
    bool requires_base = true;
};

JSON_IMPL(EnemyPrefab, type, base_model_path, attachment_model_path, hurt_path, max_health, max_speed, scale, drops);

entt::entity instance_prefab(Scene*, const EnemyPrefab&, v3i location);
bool inspect(EnemyPrefab*);
void enemy_aggro_system(Scene* scene);
void travel_system(Scene* scene);
void enemy_ik_controller_system(Scene* scene);

v3 predict_pos(Traveler& traveler, v3 pos, float time);

float get_foot_height(Scene* scene, v3 enemy_origin, v2 foot_pos);

void on_enemy_destroy(Scene& scene, entt::registry& registry, entt::entity entity);

}
