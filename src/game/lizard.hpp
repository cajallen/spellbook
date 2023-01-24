#pragma once

#include <entt/fwd.hpp>

#include "ability.hpp"
#include "general/string.hpp"
#include "general/json.hpp"
#include "general/geometry.hpp"
#include "game/stat.hpp"
#include "game/game_file.hpp"
#include "general/id_ptr.hpp"

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

struct Lizard { // Component
    LizardType type;

    id_ptr<Ability> basic_ability;

    Stat damage_multiplier = Stat(1.0f);
};


JSON_IMPL(LizardPrefab, type, model_path);

struct Scene;
entt::entity instance_prefab(Scene*, const LizardPrefab&, v3i location);
bool inspect(LizardPrefab*);

void lizard_system(Scene* scene);
void projectile_system(Scene* scene);

}
