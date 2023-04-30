#pragma once

#include <entt/entity/fwd.hpp>

#include "targeting.hpp"
#include "game/timer.hpp"
#include "game/entities/stat.hpp"

namespace spellbook {

struct Scene;
struct Caster;
struct Ability;

float instant_time_to_callback(Ability&, v3i);

struct Ability {
    enum Type {
        Type_Attack,
        Type_Ability
    };
    
    string name;

    bool set_attack_anims;
    bool set_cast_anims;
    bool auto_remove_dragging_impair;
    Scene* scene;
    entt::entity caster;
    
    bool has_target;
    v3i target;
    
    StatInstance pre_trigger_time;
    StatInstance post_trigger_time;
    
    Timer* pre_trigger_timer;
    Timer* post_trigger_timer;

    EntryGatherFunction entry_gather_function;
    
    virtual void setup(Scene* init_scene, entt::entity init_caster, float pre, float post, Type type);

    virtual void targeting();
    virtual void start();
    virtual void trigger();
    virtual void end();
    virtual float time_to_hit(v3i pos);

    virtual ~Ability();
    
    void request_cast();
    bool casting();
    bool ready_to_cast();
    void stop_casting();

    void lizard_turn_to_target();
};

void inspect(Ability* ability);

}
