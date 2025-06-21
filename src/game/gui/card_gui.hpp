#pragma once

#include "general/math/geometry.hpp"
#include "game/entities/lizards/lizard.hpp"

namespace spellbook {

struct GUIManager;
struct RenderScene;
struct Scene;

struct LizardCardGUI {
    v2i size;

    uint64 card_id;
    uint64 attack_id;
    uint64 spell_id;

    int32 depth;

    uint8 variant;
    LizardDatabase::LizardEntry& lizard;
    LizardDatabase::AbilityEntry& attack;
    LizardDatabase::AbilityEntry& spell;

    float selected_at;

    bool attack_expanded;
    bool spell_expanded;

    function<void(Scene*,v3i)> instance_function;

    // set by the UI
    v2i position;

    LizardCardGUI(LizardDatabase::LizardEntry& lizard, uint8 variant, LizardDatabase::AbilityEntry& attack, LizardDatabase::AbilityEntry& spell, int32 depth, function<void(Scene*,v3i)> instance_func);

    void draw(GUIManager* manager);
    void update(GUIManager* manager);

    v2i calculate_used_position() const;
};

}