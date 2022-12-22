#pragma once

#include <entt/fwd.hpp>

#include "general/string.hpp"
#include "general/json.hpp"
#include "general/geometry.hpp"
#include "game/stat.hpp"
#include "game/game_file.hpp"

namespace spellbook {

enum LizardType {
    LizardType_Empty,
    LizardType_Bulwark,
    LizardType_Rager,
    LizardType_Thief,
    LizardType_Assassin,
    LizardType_Trapper,
    LizardType_SacrificeSupport,
    LizardType_IllusionMage,
    LizardType_StormMage,
    LizardType_MindMage,
    LizardType_DiceMage,
    LizardType_Warlock
};

struct LizardPrefab {
    LizardType type = LizardType_Empty;
    string model_path;
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

struct Lizard { // Component
    LizardType type;
};


JSON_IMPL(LizardPrefab, type, model_path);

struct Scene;
entt::entity instance_prefab(Scene*, const LizardPrefab&, v3i location);
void inspect(LizardPrefab*);

void lizard_system(Scene* scene);
void projectile_system(Scene* scene);

}