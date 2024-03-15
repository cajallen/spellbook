#pragma once

#include <entt/entity/fwd.hpp>

#include "targeting.hpp"
#include "game/timer.hpp"
#include "game/entities/stat.hpp"
#include "renderer/assets/animation_state.hpp"

namespace spellbook {

struct Scene;
struct Ability;

float instant_time_to_callback(Ability&, v3i);

enum AbilityType {
    AbilityType_Attack,
    AbilityType_Spell
};

struct Ability {
    Scene* scene = nullptr;
    entt::entity caster = entt::null;
    
    bool set_anims = true;
    
    StatInstance pre_trigger_time;
    StatInstance post_trigger_time;
    
    std::shared_ptr<Timer> pre_trigger_timer;
    std::shared_ptr<Timer> post_trigger_timer;

    EntryEvalFunction entry_eval_function;
    EntryGatherFunction entry_gather_function;
    
    bool has_target = false;
    v3i target = {};

    Ability(Scene* _scene, entt::entity _caster);
    virtual ~Ability() = default;

    void setup_time(float base_pre, float base_post);

    virtual void request_cast();
    bool casting() const;
    void stop_casting();

    void lizard_turn_to_target();

    virtual void targeting() {}
    virtual void start() {}
    virtual void trigger() {}
    virtual void end() {}
    virtual float time_to_hit(v3i pos);
    virtual bool can_cast() const;
    
    virtual AnimationState get_pre_trigger_animation_state() const { return AnimationState_Idle; }
    virtual AnimationState get_post_trigger_animation_state() const { return AnimationState_Idle; }
};

struct Attack : Ability {
    StatInstance cooldown_time;
    std::shared_ptr<Timer> cooldown_timer;

    Attack(Scene* init_scene, entt::entity init_caster);
    virtual ~Attack() override = default;
    void setup_cd(float base_cd);
    AnimationState get_pre_trigger_animation_state() const override { return AnimationState_AttackInto; }
    AnimationState get_post_trigger_animation_state() const override { return AnimationState_AttackOut; }
    bool can_cast() const override;
};

struct Spell : Ability {
    void request_cast() override;

    Spell(Scene* init_scene, entt::entity init_caster);
    virtual ~Spell() override = default;
    AnimationState get_pre_trigger_animation_state() const override { return AnimationState_CastInto; }
    AnimationState get_post_trigger_animation_state() const override { return AnimationState_CastOut; }
};

void inspect(Ability* ability);

}
