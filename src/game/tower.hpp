#pragma once

#include <entt/fwd.hpp>

#include "string.hpp"
#include "json.hpp"
#include "geometry.hpp"

#include "stat.hpp"

namespace spellbook {

enum TowerType {
    TowerType_Empty,
    TowerType_Earth,
    TowerType_Archipelago,
    TowerType_Desert,
    TowerType_Energy,
    TowerType_Arctic,
    TowerType_Lava
};

struct TowerPrefab {
    TowerType type = TowerType_Empty;
    string globe_path;
    string clouds_path;
    string file_path;
};

struct Projectile { // Component
    entt::entity parent = {};
    v3           velocity = v3(0.0f);
    f32          lifetime = 0.0f;
};

struct ProjectileAttack {
    f32 last_tick = -FLT_MAX;
    f32 rate = 5.0f;
    f32 damage = 0.0f;
    
    f32 projectile_speed = 3.0f;
    f32 projectile_radius = 0.2f;
    f32 projectile_lifetime = 1.0f;
    int projectile_pierce = 1;
    f32 projectile_growth = 0.0f;
};

struct ZoneAttack {
    f32 last_tick = -FLT_MAX;
    f32 rate = 0.5f;
    f32 damage = 1.0f; 
    
    f32 radius = 3.0f;
};

struct TargetAttack {
    f32 last_tick = -FLT_MAX;
    f32 rate = 0.5f;
    f32 damage = 0.0f;
    
    f32 radius = 3.0f;
};

struct Tower { // Component
    f32 rotation_speed = 1.0f;
    f32 current_rotation = 0.0f;

    entt::entity clouds;

    TowerType type;
};

struct Earth { // Component
    ProjectileAttack attack;
};

struct Archipelago { // Component
    ProjectileAttack attack;
};

struct Desert { // Component
    ZoneAttack attack;
};

struct Energy { // Component
    TargetAttack attack;
};

struct Arctic { // Component
    StatEffect slow = {.type = StatEffect::Type_Multiply, .value = -0.1, .max_stacks = 10, .until = 1.5f};
    ZoneAttack attack;
};

struct Lava { // Component
    StatEffect burn = {.type = StatEffect::Type_Add, .value = 1.0, .max_stacks = INT_MAX, .until = 4.0f};
    ProjectileAttack attack;
};


JSON_IMPL(TowerPrefab, type, globe_path, clouds_path);

struct Scene;
entt::entity instance_prefab(Scene*, const TowerPrefab&, v3i location);
void inspect(TowerPrefab*);
void save_tower(const TowerPrefab&);
TowerPrefab load_tower(const string& input_path);

void tower_system(Scene* scene);
void projectile_system(Scene* scene);

}