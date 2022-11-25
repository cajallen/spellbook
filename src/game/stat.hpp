#pragma once

#include "game/input.hpp"
#include "lib/math.hpp"
#include "lib/umap.hpp"

namespace spellbook {

struct StatEffect {
    enum Type { Type_Base, Type_Multiply, Type_Add, Type_Override };

    Type type;
    f32  value;
    int max_stacks = 1;
    float until = FLT_MAX;
    int  stacks = 1;

};

struct Stat {
    umap<string, StatEffect> effects;

    f32 value();

    Stat() {}
    Stat(f32 initial) {
        effects["ctor"] = {.type = StatEffect::Type_Base, .value = initial};
    }
};

void inspect(Stat* stat);

}
