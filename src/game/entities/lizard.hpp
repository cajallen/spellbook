﻿#pragma once

#include <entt/entity/fwd.hpp>

#include "ability.hpp"
#include "general/string.hpp"
#include "general/json.hpp"
#include "general/geometry.hpp"
#include "game/game_file.hpp"
#include "game/entities/stat.hpp"
#include "general/id_ptr.hpp"

namespace spellbook {

enum LizardType {
    LizardType_Empty,
    LizardType_Bulwark,
    LizardType_Barbarian,
    LizardType_Champion,
    LizardType_Thief,
    LizardType_Assassin,
    LizardType_Ranger,
    LizardType_SacrificeSupport,
    LizardType_IllusionMage,
    LizardType_MindMage,
    LizardType_WaterMage,
    LizardType_Warlock
};

struct LizardPrefab {
    string file_path;
    vector<string> dependencies;
    
    LizardType type = LizardType_Empty;
    string model_path;
    
    v3 default_direction = v3(0,1,0);
    f32 max_health = 1.0f;
    f32 health_regen = 0.1f;
    string hurt_path;

    float scale = 1.0f;
};

struct Lizard { // Component
    LizardType type = LizardType_Empty;
    v3 default_direction = v3(0,1,0);
};


JSON_IMPL(LizardPrefab, type, model_path, max_health, health_regen, default_direction, scale);

struct Scene;
entt::entity instance_prefab(Scene*, const LizardPrefab&, v3i location);
bool inspect(LizardPrefab*);

void draw_lizard_dragging_preview(Scene* scene, entt::entity);

}
