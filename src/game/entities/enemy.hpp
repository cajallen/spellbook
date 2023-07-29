#pragma once

#include <entt/entity/fwd.hpp>

#include "general/json.hpp"
#include "general/math/geometry.hpp"
#include "general/navigation_path.hpp"
#include "game/shop.hpp"
#include "game/game_path.hpp"
#include "game/entities/stat.hpp"

namespace spellbook {

struct Scene;

enum EnemyType {
    EnemyType_Empty,
    EnemyType_Laser,
    EnemyType_Resistor,
    EnemyType_Mortar,
    EnemyType_Ballista,
    EnemyType_Bomb
};

struct EnemyPrefab {
    FilePath file_path;
    vector<FilePath> dependencies;
    
    EnemyType type = EnemyType_Empty;
    FilePath base_model_path = "models/enemy_spider/spider_base.sbmod"_rp;
    FilePath attachment_model_path;
    FilePath hurt_path;
    
    float max_health = 10.0f;
    float max_speed = 0.75f;
    float base_scale = 0.6f;
    float attachment_scale = 0.6f;

    // just use the component directly lol
    DropChance drops;
};

struct Traveler {
    struct Target {
        uint32 priority = INT_MAX;
        v3i pos;
    };
    
    std::unique_ptr<Stat> max_speed;

    Target target = {};
    Path path = {};
    
    void set_target(Target new_target) {
        assert_else(new_target.priority != INT_MAX);
        if (new_target.priority > target.priority)
            return;
        target = new_target;
    }
    void reset_target() {
        target = {INT_MAX, v3i{}};
    }
    bool has_target() const {
        return target.priority != INT_MAX;
    }
};

struct Enemy {
    entt::entity attachment;
    entt::entity from_spawner;
    entt::entity target_consumer;
};

struct Attachment {
    entt::entity base;
    bool requires_base = true;
};

JSON_IMPL(EnemyPrefab, type, base_model_path, attachment_model_path, hurt_path, max_health, max_speed, base_scale, attachment_scale, drops);

entt::entity instance_prefab(Scene*, const EnemyPrefab&, v3i location);
bool inspect(EnemyPrefab*);
void enemy_aggro_system(Scene* scene);
void traveler_reset_system(Scene* scene);
void travel_system(Scene* scene);
void enemy_ik_controller_system(Scene* scene);
void enemy_decollision_system(Scene* scene);
void attachment_transform_system(Scene* scene);

v3 predict_pos(Traveler& traveler, v3 pos, float time);

void disconnect_attachment(Scene* scene, entt::entity base);
void connect_attachment(Scene* scene, entt::entity base, entt::entity attachment);

void on_enemy_destroy(Scene& scene, entt::registry& registry, entt::entity entity);

void inspect(Traveler* traveler);

}
