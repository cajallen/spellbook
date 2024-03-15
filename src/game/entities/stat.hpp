#pragma once

#include <entt/entity/fwd.hpp>

#include "general/input.hpp"
#include "general/umap.hpp"
#include "general/math/math.hpp"

namespace spellbook {

struct Scene;
struct EmitterCPU;

struct StatEffect {
    enum Type { Type_Base, Type_Multiply, Type_Add, Type_Override };

    Type type;
    float  value;
    int32 max_stacks = 1;
    float until = FLT_MAX; // Duration is used if adding stacks refreshes duration
    int32  stacks = 1;
    uint64 unique = 0; // If multiple buffs of this id, take best
};

struct Stat {
    Scene* scene = nullptr;
    entt::entity entity;
    umap<uint64, StatEffect> effects;

    float value();
    void add_effect(uint64 id, const StatEffect& effect, EmitterCPU* emitter = nullptr);
    umap<uint64, StatEffect>::iterator remove_effect(umap<uint64, StatEffect>::iterator it);
    void remove_effect(uint64 id);

    Stat() {}
    Stat(Scene* scene, entt::entity e) : scene(scene), entity(e) {}
    Stat(Scene* scene, entt::entity e, float initial) : scene(scene), entity(e) {
        effects[0] = {.type = StatEffect::Type_Base, .value = initial};
    }
};

struct StatInstance {
    Stat* stat = nullptr;
    float instance_base = 0.0f;
    float value() const;
};

float stat_instance_value(Stat* stat, float instance_base);

void inspect(Stat* stat);

}
