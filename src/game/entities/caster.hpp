﻿#pragma once

#include "ability.hpp"
#include "general/id_ptr.hpp"


namespace spellbook {

struct Taunt {
    enum Type {
        Type_Inactive,
        Type_Position,
        Type_UnitPosition
    };

    Scene* scene = nullptr;
    Type type = Type_Inactive;
    u64 source;
    v3i position;
    entt::entity unit;

    Taunt() {}
    Taunt(Scene* init_scene) : scene(init_scene) {}
    
    bool try_get(v3i* pos);
    bool set(u64 id, v3i pos);
    bool set(u64 id, entt::entity unit);
    bool reset(u64 id);
};

struct Caster {
    std::unique_ptr<Ability> attack;
    std::unique_ptr<Ability> ability;

    Taunt taunt;
    
    // Multipliers
    std::unique_ptr<Stat> damage;
    std::unique_ptr<Stat> heal;
    std::unique_ptr<Stat> attack_speed;
    std::unique_ptr<Stat> cooldown_reduction;
    std::unique_ptr<Stat> projectile_speed;
    std::unique_ptr<Stat> range;
    std::unique_ptr<Stat> buff_duration;
    std::unique_ptr<Stat> debuff_duration;
    std::unique_ptr<Stat> lifesteal;

    Caster(Scene* scene);

    bool casting() const;
};

void caster_system(Scene* scene);

}