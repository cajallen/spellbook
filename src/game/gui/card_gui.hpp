#pragma once

#include "general/math/geometry.hpp"
#include "game/entities/lizards/lizard.hpp"

namespace spellbook {

struct GUIManager;
struct RenderScene;
struct Scene;

struct CardSettings {
    int32 expand_size = 170;
    v2i padding = {20, 20};
    int32 line_spacing = 30;
};

struct LizardCardGUI {
    enum Expanded {
        Expanded_None,
        Expanded_Attack,
        Expanded_Spell
    };

    CardSettings settings;

    v2i size;
    uint64 untextured_mat;

    int32 depth;

    LizardDatabase::LizardEntry& lizard;
    LizardDatabase::AbilityEntry& attack;
    LizardDatabase::AbilityEntry& spell;

    float selected_at;

    Expanded expanded;

    std::function<void(Scene*,v3i)> instance_function;

    // set by the UI
    v2i position;

    LizardCardGUI(LizardDatabase::LizardEntry& lizard, LizardDatabase::AbilityEntry& attack, LizardDatabase::AbilityEntry& spell, int32 depth, const std::function<void(Scene*,v3i)>& instance_func);

    void draw(GUIManager* manager, RenderScene& render_scene) const;
    void update(GUIManager* manager);

    v2i calculate_used_position() const;
    range2i calculate_attack_title_region() const;
    range2i calculate_spell_title_region() const;
    v2i calculate_attack_title_offset() const;
    v2i calculate_spell_title_offset() const;

    v2i calculate_attack_text_offset() const;
    v2i calculate_spell_text_offset() const;
};

}