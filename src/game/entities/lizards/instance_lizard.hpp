#pragma once

#include "general/math/geometry.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/lizards/lizard.hpp"
#include "game/gui/shop_gui.hpp"

namespace spellbook {

template <LizardType LT, uint8 Variant>
void instance_lizard(Scene* scene, v3i location) {
    static int i      = 0;
    auto& lizard_entry = get_lizard_database().lizard_entries[LT];
    auto       entity = setup_basic_unit(scene, lizard_entry.model_path, v3(location), 10.0f, lizard_entry.hurt_emitter);

    string lizard_name = fmt_("{},{}", magic_enum::enum_name(LT).substr(strlen("LizardType_")), lizard_entry.variants[Variant].title);
    scene->registry.emplace<Name>(entity, lizard_name);

    scene->registry.emplace<Caster>(entity, scene, entity);
    scene->registry.emplace<Draggable>(entity, 0.5f);

    ModelTransform& model_tfm = scene->registry.get<ModelTransform>(entity);
    model_tfm.set_scale(v3(lizard_entry.scale));

    PoseController* poser = scene->registry.try_get<PoseController>(entity);
    if (poser)
        poser->set_state(AnimationState_Idle, 0.0f);
}

// Idk why this is here but it works
template<LizardType LT, uint8 Variant>
void add_lizard_card(ShopGUI& shop) {
    auto& lizard_entry = get_lizard_database().lizard_entries.at(LT);
    auto& attack_entry = get_lizard_database().ability_entries.at(ability_index(LT, Variant, AbilityType_Attack));
    auto& spell_entry = get_lizard_database().ability_entries.at(ability_index(LT, Variant, AbilityType_Spell));

    LizardCardGUI card(lizard_entry, Variant, attack_entry, spell_entry, 500, std::function<void(Scene*,v3i)>(instance_lizard<LT, Variant>));

    shop.cards.push_back(card);
}

}