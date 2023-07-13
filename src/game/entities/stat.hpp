#pragma once

#include "game/input.hpp"
#include "general/umap.hpp"
#include "general/math/math.hpp"

namespace spellbook {

struct Scene;

struct StatEffect {
    enum Type { Type_Base, Type_Multiply, Type_Add, Type_Override };

    Type type;
    float  value;
    int max_stacks = 1;
    float until = FLT_MAX; // Duration is used if adding stacks refreshes duration
    int  stacks = 1;

};

struct Stat {
    Scene* scene = nullptr;
    umap<uint64, StatEffect> effects;

    float value();
    void add_effect(uint64 id, const StatEffect& effect);
    void remove_effect(uint64 id);

    Stat() {}
    Stat(Scene* scene) : scene(scene) {}
    Stat(Scene* scene, float initial) : scene(scene) {
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
