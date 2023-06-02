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

struct Ability {
    Scene* scene;
    entt::entity caster;
    
    bool set_anims;
    
    StatInstance pre_trigger_time;
    StatInstance post_trigger_time;
    
    std::shared_ptr<Timer> pre_trigger_timer;
    std::shared_ptr<Timer> post_trigger_timer;

    StatInstance range;
    EntryGatherFunction entry_gather_function;
    
    bool has_target;
    v3i target;

    Ability(Scene* init_scene, entt::entity init_caster, float pre, float post, float range = 1000.0f);
    virtual ~Ability() = default;
    
    virtual void request_cast();
    bool casting() const;
    void stop_casting();

    void lizard_turn_to_target();

    virtual void targeting() {}
    virtual void start() {}
    virtual void trigger() {}
    virtual void end() {}
    virtual float time_to_hit(v3i pos);
    virtual bool in_range() const;
    virtual bool can_cast() const;
    
    virtual string get_name() const { return "ability"; }
    virtual AnimationState get_pre_trigger_animation_state() const { return AnimationState_Idle; }
    virtual AnimationState get_post_trigger_animation_state() const { return AnimationState_Idle; }
};

struct Attack : Ability {
    StatInstance cooldown_time;
    std::shared_ptr<Timer> cooldown_timer;

    Attack(Scene* init_scene, entt::entity init_caster, float pre, float post, float cooldown, float range = 1000.0f);
    ~Attack() override = default;
    string get_name() const override { return "attack"; }
    AnimationState get_pre_trigger_animation_state() const override { return AnimationState_AttackInto; }
    AnimationState get_post_trigger_animation_state() const override { return AnimationState_AttackOut; }
    bool can_cast() const override;
};

struct Spell : Ability {
    void request_cast() override;

    using Ability::Ability;
    
    ~Spell() override = default;
    string get_name() const override { return "spell"; }
    AnimationState get_pre_trigger_animation_state() const override { return AnimationState_CastInto; }
    AnimationState get_post_trigger_animation_state() const override { return AnimationState_CastOut; }
};

void inspect(Ability* ability);

}
