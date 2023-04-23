#pragma once

#include "game/input.hpp"
#include "general/math.hpp"
#include "general/umap.hpp"

namespace spellbook {

struct Scene;

struct StatEffect {
    enum Type { Type_Base, Type_Multiply, Type_Add, Type_Override };

    Type type;
    f32  value;
    int max_stacks = 1;
    float until = FLT_MAX; // Duration is used if adding stacks refreshes duration
    int  stacks = 1;

};

struct Stat {
    Scene* scene = nullptr;
    umap<u64, StatEffect> effects;

    f32 value();
    void add_effect(u64 id, const StatEffect& effect);
    void remove_effect(u64 id);

    Stat() {}
    Stat(Scene* scene) : scene(scene) {}
    Stat(Scene* scene, f32 initial) : scene(scene) {
        effects[0] = {.type = StatEffect::Type_Base, .value = initial};
    }
};

struct StatInstance {
    Stat* stat = nullptr;
    float instance_base = 0.0f;
    float value();
};

float stat_instance_value(Stat* stat, float instance_base);

void inspect(Stat* stat);

}
