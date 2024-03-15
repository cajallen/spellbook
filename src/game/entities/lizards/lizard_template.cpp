#include "game/entities/lizards/lizard.hpp"

#include <entt/entity/entity.hpp>

#include "game/scene.hpp"
#include "game/entities/lizard.hpp"

namespace spellbook {

struct PlaceholderAttack : Attack {
    using Attack::Attack;
    void targeting() override;
    void start() override;
    void trigger() override;
    float time_to_hit(v3i pos) override;

    string get_name() const override { return "Placeholder Attack"; }
};

void PlaceholderAttack::start() {
    lizard_turn_to_target();
}

void PlaceholderAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);

    if (taunted(*this, caster_comp))
        return;

    if (square_targeting(1, *this, entry_gather_function, entry_eval_function))
        return;
}

void PlaceholderAttack::trigger() {
    lizard_turn_to_target();
}

void PlaceholderAttack::time_to_hit() {
    lizard_turn_to_target();
}

struct PlaceholderSpell : Spell {
    using Spell::Spell;
    void start() override;
    void trigger() override;
    void targeting() override;
    void end() override;

    string get_name() const override { return "Placeholder Spell"; }
};

void RangerSpell::start() {
    lizard_turn_to_target();
}

void RangerSpell::trigger() {
    lizard_turn_to_target();
}

void RangerSpell::targeting() {
    lizard_turn_to_target();
}

void RangerSpell::end() {
    lizard_turn_to_target();
}

void build_placeholder(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);

    Caster& caster = scene->registry.get<Caster>(entity);

    caster.attack = std::make_unique<PlaceholderAttack>(scene, entity, 1.0f, 1.0f, 1.0f, 1.0f);
    caster.attack->entry_gather_function = gather_enemies();
    caster.attack->entry_eval_function = basic_lizard_entry_eval;

    caster.spell = std::make_unique<PlaceholderSpell>(scene, entity, 1.0f, 1.0f);
    caster.spell->entry_gather_function = gather_enemies();
    caster.spell->entry_eval_function = basic_lizard_entry_eval;
}

template <>
string get_name<LizardType_Placeholder>() {
    return "Placeholder";
}

template <>
string get_attack_title<LizardType_Placeholder>() {
    return PlaceholderAttack().get_name();
}

template <>
string get_spell_title<LizardType_Placeholder>() {
    return PlaceholderSpell().get_name();
}

}
