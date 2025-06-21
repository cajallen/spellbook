#pragma once

#include <entt/entity/fwd.hpp>

#include "general/memory.hpp"
#include "general/math/geometry.hpp"
#include "game/entities/ability.hpp"

namespace spellbook {

struct Scene;
struct Stat;

struct Taunt {
    enum Type {
        Type_Inactive,
        Type_Position,
        Type_UnitPosition
    };

    Scene* scene = nullptr;
    Type type = Type_Inactive;
    uint64 source;
    v3i position;
    entt::entity unit;

    Taunt() {}
    Taunt(Scene* init_scene) : scene(init_scene) {}
    
    bool try_get(v3i* pos);
    bool set(uint64 id, v3i pos);
    bool set(uint64 id, entt::entity unit);
    bool reset(uint64 id);
};

struct Caster {
    unique_ptr<Attack> attack;
    unique_ptr<Spell> spell;

    Taunt taunt;
    
    // Multipliers
    unique_ptr<Stat> damage;
    unique_ptr<Stat> heal;
    unique_ptr<Stat> attack_speed;
    unique_ptr<Stat> cooldown_speed;
    unique_ptr<Stat> projectile_speed;
    unique_ptr<Stat> range;
    unique_ptr<Stat> buff_duration;
    unique_ptr<Stat> debuff_duration;
    unique_ptr<Stat> lifesteal;

    Caster(Scene* scene, entt::entity e);

    bool casting() const;
};

void caster_system(Scene* scene);
void on_caster_destroy(Scene& scene, entt::registry& registry, entt::entity entity);

}