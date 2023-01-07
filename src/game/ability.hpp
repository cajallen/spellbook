#pragma once

#include <entt/fwd.hpp>

#include "game/timer.hpp"
#include "game/stat.hpp"
#include "general/id_ptr.hpp"

namespace spellbook {

struct Scene;

struct Ability {
    string name;

    Scene* scene;
    entt::entity caster;
    
    bool has_target;
    v3i target;
    
    Stat pre_trigger_time;
    Stat post_trigger_time;
    Stat cooldown_time;
    
    Timer* pre_trigger_timer;
    Timer* post_trigger_timer;
    Timer* cooldown_timer;

    void(*targeting_callback)(void*) = {};
    void* targeting_payload = nullptr;
    void(*start_callback)(void*) = {};
    void* start_payload = nullptr;
    void(*trigger_callback)(void*) = {};
    void* trigger_payload = nullptr;
    void(*end_callback)(void*) = {};
    void* end_payload = nullptr;
    void(*ready_callback)(void*) = {};
    void* ready_payload = nullptr;
    
    Ability() {}

    void request_cast();
    bool casting() { return pre_trigger_timer->ticking || post_trigger_timer->ticking; }
    bool ready_to_cast() { return !cooldown_timer->ticking && !casting(); }
};

id_ptr<Ability> make_ability(Scene* scene, const string& set_name);
void inspect(Ability* ability);

}
