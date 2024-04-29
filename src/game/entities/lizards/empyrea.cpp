#include "game/entities/lizards/lizard.hpp"

#include <entt/entity/entity.hpp>
#include <entt/core/hashed_string.hpp>

#include "extension/fmt.hpp"
#include "general/file/file_path.hpp"
#include "editor/console.hpp"
#include "renderer/draw_functions.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/targeting.hpp"
#include "game/effects/hit.hpp"
#include "game/effects/hit_effect.hpp"

namespace spellbook {

using namespace entt::literals;


template<uint8 variant>
struct EmpyreaAttack : Attack {
    shared_ptr<Timer> ethereal_cooldown;
    LizardDatabase::AbilityEntry* entry;
    uint32 phase = 0;


    void EtherealReaction(Scene* scene, entt::entity propagator, entt::entity receiver, HitEffect& effect);
    void DamageReaction(Scene* scene, entt::entity propagator, entt::entity receiver, HitEffect& effect);
    void ExecuteReaction(Scene* scene, entt::entity propagator, entt::entity receiver, HitEffect& effect);

    EmpyreaAttack(Scene* _scene, entt::entity _caster);
    ~EmpyreaAttack() override {
        if (!scene)
            return;
        scene->hit_scene_state.cleanup_on_death(caster);
    }
    void start() override {
        auto poser = scene->registry.try_get<PoseController>(caster);
        if (poser) {
            poser->set_state(phase == 0 ? AnimationState_AttackInto : AnimationState_Attack2Into, pre_trigger_time.value());
        }
        lizard_turn_to_target();
    }

    void trigger() override;

    void end() override {
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

    void targeting() override {
        Caster& caster_comp = scene->registry.get<Caster>(caster);

        if (taunted(*this, caster_comp))
            return;

        plus_targeting(1, *this, entry_gather_function, entry_eval_function);
    }
};

template<uint8 variant>
struct EmpyreaSpell : Spell {
    LizardDatabase::AbilityEntry* entry;
    LizardDatabase::AbilityEntry* attack_entry;

    EmpyreaSpell(Scene* _scene, entt::entity _caster) : Spell(_scene, _caster) {
        entry = &get_lizard_database().ability_entries.at(ability_index(LizardType_Empyrea, variant, AbilityType_Spell));
        attack_entry = &get_lizard_database().ability_entries.at(ability_index(LizardType_Empyrea, variant, AbilityType_Attack));
        setup_time(entry->pre_time, entry->post_time);
        entry_gather_function = gather_enemies();
        entry_eval_function = basic_lizard_entry_eval;
    }

    void trigger() override;
};


/// CORROSIVE
template<uint8 variant>
void EmpyreaAttack<variant>::DamageReaction(Scene* scene, entt::entity propagator, entt::entity receiver, HitEffect& effect) {
    if (ethereal_cooldown->ticking)
        return;

    HitEffect ethereal_effect(EffectType_Ethereal);
    ethereal_effect.duration = entry->extra_floats.at("EtherealDuration");

    Hit hit = {
        .scene = scene,
        .position = math::round_cast(scene->registry.get<LogicTransform>(caster).position),
        .caster = caster,
        .effects = {ethereal_effect}
    };
    hit.process();

    ethereal_cooldown->start(entry->extra_floats.at("EtherealCooldown"));
}

template<>
EmpyreaAttack<0>::EmpyreaAttack(Scene* _scene, entt::entity _caster) : Attack(_scene, _caster) {
    entry = &get_lizard_database().ability_entries.at(ability_index(LizardType_Empyrea, 0, AbilityType_Attack));
    setup_time(entry->pre_time, entry->post_time);
    setup_cd(entry->extra_floats.at("CD"));
    ethereal_cooldown = add_timer(scene, [](Timer* timer) {});

    HitReaction& damage_reaction = scene->hit_scene_state.reactions.push_back({});
    damage_reaction.relies_on_effect_type = EffectType_Damage;
    damage_reaction.relies_on_receiver = caster;
    damage_reaction.reaction.connect<&EmpyreaAttack<0>::DamageReaction>(*this);

    entry_gather_function = gather_enemies();
    entry_eval_function = basic_lizard_entry_eval;
    set_anims = false;
}

template<>
void EmpyreaAttack<0>::trigger() {
    auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
    v3 hit_vec = v3(target) - logic_tfm.position;

    EmitterCPU emitter_cpu = load_resource<EmitterCPU>("emitters/empyrea/basic_hit.sbjemt"_resource);
    emitter_cpu.orientation_settings.rotation = math::quat_between(v3(1.0f, 0.0f, 0.0f), math::normalize(hit_vec));
    quick_emitter(scene, "Empyrea hit", v3(target) + v3(0.5f), emitter_cpu, 0.2f);

    HitEffect damage_effect(EffectType_Damage);
    damage_effect.quantity = entry->extra_floats.at("Damage");
    HitEffect acid_effect(EffectType_Acid);
    acid_effect.level = 0;
    acid_effect.duration = entry->extra_floats.at("AcidDuration");

    Hit hit = {
        .scene = scene,
        .position = target,
        .caster = caster,
        .effects = {damage_effect, acid_effect}
    };
    hit.process();

    auto poser = scene->registry.try_get<PoseController>(caster);
    if (poser) {
        poser->set_state(phase == 0 ? AnimationState_AttackOut : AnimationState_Attack2Out, post_trigger_time.value());
    }

    phase = phase == 0 ? 1 : 0;
}

template<>
void EmpyreaSpell<0>::trigger() {
    auto& logic_tfm = scene->registry.get<LogicTransform>(caster);

    HitEffect floor_effect(EffectType_Floor);
    HitEffect custom_effect(EffectType_Custom);
    custom_effect.hits_disposition_function = [](Team::Disposition disposition) {
        switch (disposition) {
            case Team::Disposition_Friendly: return false;
            case Team::Disposition_Hostile: return true;
            default: return false;
        }
    };
    custom_effect.apply_function = [this](HitEffect& effect, Scene* scene, entt::entity caster_arg, const vector<entt::entity>& hit_targets) {
        if (!scene->registry.valid(caster_arg)) // dead
            return;
        for (entt::entity hit_target : hit_targets) {
            Acid* acid = scene->registry.try_get<Acid>(hit_target);
            if (!acid)
                continue;

            console({.str="Empyrea Spell Spread"});

            acid->tick_effects.emplace_back(
                scene->time + entry->extra_floats.at("CatalysisDuration"),
                [this](Scene* scene, entt::entity target) -> void {
                    LogicTransform& logic_tfm = scene->registry.get<LogicTransform>(target);

                    HitEffect floor_effect(EffectType_Floor);
                    HitEffect slow_effect(EffectType_Slow);
                    slow_effect.duration = entry->extra_floats.at("SlowDuration");
                    slow_effect.level = math::round_cast(entry->extra_floats.at("SlowLevel"));
                    HitEffect acid_effect(EffectType_Acid);
                    acid_effect.level = 1;
                    acid_effect.duration = attack_entry->extra_floats.at("AcidDuration");
                    Hit hit = {
                        .scene = scene,
                        .position = math::round_cast(logic_tfm.position),
                        .caster = caster,
                        .effects = {floor_effect, slow_effect, acid_effect}
                    };
                    hit.effects.push_back(acid_effect);
                    hit.process();
                }
            );
        }
    };
    Hit hit = {
        .scene = scene,
        .position = math::round_cast(logic_tfm.position),
        .caster = caster,
        .effects = {floor_effect, custom_effect}
    };
    hit.effects.push_back(custom_effect);
    hit.process();
}


/// SHADOW DANCER
template<uint8 variant>
void EmpyreaAttack<variant>::EtherealReaction(Scene* scene, entt::entity propagator, entt::entity receiver, HitEffect& effect) {
    HitEffect amplify_effect(EffectType_Amplify);
    amplify_effect.level = math::round_cast(entry->extra_floats.at("AmplifyLevel"));
    amplify_effect.duration = entry->extra_floats.at("AmplifyDuration");
    Hit hit = {
                        .scene = scene,
                        .position = math::round_cast(scene->registry.get<LogicTransform>(caster).position),
                        .caster = caster,
                        .effects = {amplify_effect}
    };
    hit.process();
}

template<>
EmpyreaAttack<1>::EmpyreaAttack(Scene* _scene, entt::entity _caster) : Attack(_scene, _caster) {
    entry = &get_lizard_database().ability_entries.at(ability_index(LizardType_Empyrea, 0, AbilityType_Attack));
    setup_time(entry->pre_time, entry->post_time);
    setup_cd(entry->extra_floats.at("CD"));
    ethereal_cooldown = add_timer(scene, [](Timer* timer) {});

    HitReaction& damage_reaction = scene->hit_scene_state.reactions.push_back({});
    damage_reaction.relies_on_effect_type = EffectType_Damage;
    damage_reaction.relies_on_receiver = caster;
    damage_reaction.reaction.connect<&EmpyreaAttack<1>::DamageReaction>(*this);

    HitReaction& ethereal_reaction = scene->hit_scene_state.reactions.push_back({});
    ethereal_reaction.relies_on_effect_type = EffectType_Ethereal;
    ethereal_reaction.relies_on_receiver = caster;
    ethereal_reaction.reaction.connect<&EmpyreaAttack<1>::EtherealReaction>(*this);

    entry_gather_function = gather_enemies();
    entry_eval_function = basic_lizard_entry_eval;
    set_anims = false;
}

template<>
void EmpyreaAttack<1>::trigger() {
    auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
    v3 hit_vec = v3(target) - logic_tfm.position;

    EmitterCPU emitter_cpu = load_resource<EmitterCPU>("emitters/empyrea/basic_hit.sbjemt"_resource);
    emitter_cpu.orientation_settings.rotation = math::quat_between(v3(1.0f, 0.0f, 0.0f), math::normalize(hit_vec));
    quick_emitter(scene, "Empyrea hit", v3(target) + v3(0.5f), emitter_cpu, 0.2f);

    HitEffect damage_effect(EffectType_Damage);
    damage_effect.quantity = entry->extra_floats.at("Damage");

    Hit hit = {
        .scene = scene,
        .position = target,
        .caster = caster,
        .effects = {damage_effect}
    };
    hit.process();

    auto poser = scene->registry.try_get<PoseController>(caster);
    if (poser) {
        poser->set_state(phase == 0 ? AnimationState_AttackOut : AnimationState_Attack2Out, post_trigger_time.value());
    }

    phase = phase == 0 ? 1 : 0;
}

template<>
void EmpyreaSpell<1>::trigger() {
    auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
    HitEffect regen_effect(EffectType_Regen);
    regen_effect.quantity = entry->extra_floats.at("RegenAmount");
    regen_effect.duration = entry->extra_floats.at("RegenDuration");

    Hit hit = {
        .scene = scene,
        .position = math::round_cast(logic_tfm.position),
        .caster = caster,
        .effects = {regen_effect}
    };
    hit.process();
}


/// ASSASSIN
template<uint8 variant>
void EmpyreaAttack<variant>::ExecuteReaction(Scene* scene, entt::entity propagator, entt::entity receiver, HitEffect& effect) {
    HitEffect heal_effect(EffectType_Amplify);
    heal_effect.quantity = entry->extra_floats.at("ExecuteHealAmount");
    Hit hit = {
                     .scene = scene,
                     .position = math::round_cast(scene->registry.get<LogicTransform>(caster).position),
                     .caster = caster,
                     .effects = {heal_effect}
    };
    hit.process();
}

template<>
EmpyreaAttack<2>::EmpyreaAttack(Scene* _scene, entt::entity _caster) : Attack(_scene, _caster) {
    entry = &get_lizard_database().ability_entries.at(ability_index(LizardType_Empyrea, 0, AbilityType_Attack));
    setup_time(entry->pre_time, entry->post_time);
    setup_cd(entry->extra_floats.at("CD"));
    ethereal_cooldown = add_timer(scene, [](Timer* timer) {});

    HitReaction& execute_reaction = scene->hit_scene_state.reactions.push_back({});
    execute_reaction.relies_on_effect_type = EffectType_Execute;
    execute_reaction.relies_on_receiver = caster;
    execute_reaction.reaction.connect<&EmpyreaAttack<2>::ExecuteReaction>(*this);

    entry_gather_function = gather_enemies();
    entry_eval_function = basic_lizard_entry_eval;
    set_anims = false;
}

template<>
void EmpyreaAttack<2>::trigger() {
    auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
    v3 hit_vec = v3(target) - logic_tfm.position;

    EmitterCPU emitter_cpu = load_resource<EmitterCPU>("emitters/empyrea/basic_hit.sbjemt"_resource);
    emitter_cpu.orientation_settings.rotation = math::quat_between(v3(1.0f, 0.0f, 0.0f), math::normalize(hit_vec));
    quick_emitter(scene, "Empyrea hit", v3(target) + v3(0.5f), emitter_cpu, 0.2f);

    HitEffect damage_effect(EffectType_Damage);
    damage_effect.quantity = entry->extra_floats.at("Damage");
    HitEffect execute_effect(EffectType_Execute);
    execute_effect.quantity = entry->extra_floats.at("ExecuteAmount");

    Hit hit = {
        .scene = scene,
        .position = target,
        .caster = caster,
        .effects = {damage_effect, execute_effect}
    };
    hit.process();

    auto poser = scene->registry.try_get<PoseController>(caster);
    if (poser) {
        poser->set_state(phase == 0 ? AnimationState_AttackOut : AnimationState_Attack2Out, post_trigger_time.value());
    }

    phase = phase == 0 ? 1 : 0;
}

template<>
void EmpyreaSpell<2>::trigger() {
    HitEffect floor_effect(EffectType_Floor);
    HitEffect cull_effect(EffectType_Cull);
    cull_effect.quantity = entry->extra_floats.at("CullAmount");

    Hit hit = {
        .scene = scene,
        .position = target,
        .caster = caster,
        .effects = {floor_effect, cull_effect}
    };
    hit.process();
}



/// GENERAL
template<>
void draw_dragging_preview<LizardType_Empyrea, 0>(Scene* scene, entt::entity entity) {
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

template<>
void draw_dragging_preview<LizardType_Empyrea, 1>(Scene* scene, entt::entity entity) {
    draw_dragging_preview<LizardType_Empyrea, 0>(scene, entity);
}

template<>
void draw_dragging_preview<LizardType_Empyrea, 2>(Scene* scene, entt::entity entity) {
    draw_dragging_preview<LizardType_Empyrea, 0>(scene, entity);
}


template<>
void build<LizardType_Empyrea, 0>(Scene* scene, entt::entity entity) {
    Lizard& lizard = scene->registry.emplace<Lizard>(entity, LizardType_Empyrea, uint8(0));
    Caster& caster = scene->registry.get<Caster>(entity);

    lizard.dragging_preview_function = draw_dragging_preview<LizardType_Empyrea, 0>;
    caster.attack = std::make_unique<EmpyreaAttack<0>>(scene, entity);
    caster.spell = std::make_unique<EmpyreaSpell<0>>(scene, entity);
}

template<>
void build<LizardType_Empyrea, 1>(Scene* scene, entt::entity entity) {
    Lizard& lizard = scene->registry.emplace<Lizard>(entity, LizardType_Empyrea, uint8(1));
    Caster& caster = scene->registry.get<Caster>(entity);

    lizard.dragging_preview_function = draw_dragging_preview<LizardType_Empyrea, 1>;
    caster.attack = std::make_unique<EmpyreaAttack<1>>(scene, entity);
    caster.spell = std::make_unique<EmpyreaSpell<1>>(scene, entity);
}

template<>
void build<LizardType_Empyrea, 2>(Scene* scene, entt::entity entity) {
    Lizard& lizard = scene->registry.emplace<Lizard>(entity, LizardType_Empyrea, uint8(2));
    Caster& caster = scene->registry.get<Caster>(entity);

    lizard.dragging_preview_function = draw_dragging_preview<LizardType_Empyrea, 2>;
    caster.attack = std::make_unique<EmpyreaAttack<2>>(scene, entity);
    caster.spell = std::make_unique<EmpyreaSpell<2>>(scene, entity);
}

}
