#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/targeting.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

constexpr int poison_max_stacks = 4;
constexpr float poison_dps = 1.0f;
constexpr float poison_duration = 4.0f;

struct AssassinAttack : Attack {
    using Attack::Attack;
    
    int phase = 0;
    void targeting() override;
    void start() override;
    void trigger() override;
    void end() override;
};

void AssassinAttack::start() {
    auto poser = scene->registry.try_get<PoseController>(caster);
    if (poser) {
        if (phase == 0) {
            poser->set_state(AnimationState_AttackInto, pre_trigger_time.value());
        } else {
            poser->set_state(AnimationState_Attack2Into, pre_trigger_time.value());
        }
    }

    lizard_turn_to_target();
}

void AssassinAttack::trigger() {
    auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
    v3 hit_vec = v3(target) - logic_tfm.position;
    
    uset<entt::entity> enemies = entry_gather_function(*this, target, 0.0f);
    for (auto& enemy : enemies) {
        auto& health = scene->registry.get<Health>(enemy);
        auto& enemy_tfm = scene->registry.get<LogicTransform>(enemy);
        damage(scene, caster, enemy, 3.0f / enemies.size(), enemy_tfm.position - logic_tfm.position);

        if (!health.dots.contains(caster))
            health.dots[caster] = std::make_unique<Stat>(scene, 0.0f);
        health.dots[caster]->add_effect(0, StatEffect{StatEffect::Type_Add, poison_dps, poison_max_stacks, poison_duration});
    }

    EmitterCPU emitter_cpu = load_asset<EmitterCPU>("emitters/assassin/basic_hit.sbemt");
    emitter_cpu.rotation = math::quat_between(v3(1.0f, 0.0f, 0.0f), math::normalize(hit_vec));
    quick_emitter(scene, "Assassin hit", v3(target) + v3(0.5f), emitter_cpu, 0.2f);

    auto poser = scene->registry.try_get<PoseController>(caster);
    if (poser) {
        if (phase == 0) {
            poser->set_state(AnimationState_AttackOut, post_trigger_time.value());
        } else {
            poser->set_state(AnimationState_Attack2Out, post_trigger_time.value());
        }
    }
    if (phase == 0) {
        phase = 1;
    } else {
        phase = 0;
    }
}

void AssassinAttack::end() {
    if (phase == 0) {
        post_trigger_timer->stop();
        request_cast();
    } else {
        auto poser = scene->registry.try_get<PoseController>(caster);
        if (poser) {
            poser->set_state(AnimationState_Idle, post_trigger_time.value());
        }
    }
    
}

void AssassinAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);

    if (taunted(*this, caster_comp))
        return;
    
    plus_targeting(1, *this, entry_gather_function, entry_eval_function);
}

struct AssassinSpell : Spell {
    using Spell::Spell;

    string get_name() const override { return "Assassin Spell"; }
};

void build_assassin(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    Caster& caster = scene->registry.get<Caster>(entity);
    
    caster.attack = std::make_unique<AssassinAttack>(scene, entity, 0.5f, 0.5f, 0.1f, 1.0f);
    caster.attack->entry_gather_function = gather_enemies();
    caster.attack->entry_eval_function = basic_lizard_entry_eval;

    caster.spell = std::make_unique<AssassinSpell>(scene, entity, 1.0f, 1.0f);
    caster.spell->entry_gather_function = gather_enemies();
    caster.spell->entry_eval_function = basic_lizard_entry_eval;
}

void draw_assassin_dragging_preview(Scene* scene, entt::entity entity) {
    v3 logic_pos = scene->registry.get<Dragging>(entity).potential_logic_position;
    v3i logic_posi = math::round_cast(logic_pos);
    
    vector<v3i> center_pieces = {logic_posi};
    for (const v2i& offset : {v2i{-1, 0}, v2i{0, -1}, v2i{1, 0}, v2i{0, 1}}) {
        center_pieces.push_back(logic_posi + v3i(offset, 0));
    }
    scene->render_scene.quick_mesh(
        generate_outline(&scene->camera, scene->map_data.solids, center_pieces, palette::green_yellow, 0.05f),
        true, false);
}


}
