#pragma once

#include <entt/entity/fwd.hpp>

#include "general/id_ptr.hpp"
#include "game/timer.hpp"
#include "game/entities/stat.hpp"

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
    bool casting();
    bool ready_to_cast();
    void stop_casting();
};

id_ptr<Ability> make_ability(Scene* scene, const string& set_name);
void destroy_ability(Scene* scene, id_ptr<Ability> ability);
void inspect(Ability* ability);

}
