#pragma once

#include "card_gui.hpp"

#include "game/entities/lizards/instance_lizard.hpp"
#include "game/entities/lizards/lizard.hpp"

namespace spellbook {

struct GUIManager;

struct ShopGUI {
    vector<LizardCardGUI> cards;

    void add_lizard_card(LizardType type);

    template<LizardType T>
    void add_lizard_card() {
        auto& lizard_entry = get_lizard_database().lizard_entries.at(LizardType_Assassin);
        auto& attack_entry = get_lizard_database().ability_entries.at(ability_index(LizardType_Assassin, AbilityType_Attack));
        auto& spell_entry = get_lizard_database().ability_entries.at(ability_index(LizardType_Assassin, AbilityType_Spell));

        LizardCardGUI card(lizard_entry, attack_entry, spell_entry, 500, instance_lizard<T>);

        cards.push_back(card);
    }

    void draw(GUIManager* manager, RenderScene& render_scene);
    void update(GUIManager* manager);
};

}