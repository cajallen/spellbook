#pragma once

#include "math.hpp"
#include "umap.hpp"

namespace spellbook {

struct StatEffect {
    enum Type { Type_Base, Type_Multiply, Type_Add, Type_Override };
    Type type;
    f32  value;
    int  stacks = 1;
};

struct Stat {
    umap<string, StatEffect> effects;

    inline f32 get_stat() {
        f32 base = 0.0f;
        f32 mult = 1.0f;
        f32 add  = 0.0f;

        for (auto& [_, effect] : effects) {
            switch (effect.type) {
                case (StatEffect::Type_Base): {
                    base += effect.value * effect.stacks;
                } break;
                case (StatEffect::Type_Multiply): {
                    mult *= math::pow(1.0f + effect.value, (f32) effect.stacks);
                } break;
                case (StatEffect::Type_Add): {
                    add += effect.value * effect.stacks;
                } break;
                case (StatEffect::Type_Override): {
                    return effect.value;
                } break;
            }
        }
        return base * mult + add;
    }

    Stat() {}
    Stat(f32 initial) {
        effects["ctor"] = {.type = StatEffect::Type_Base, .value = initial};
    }
};

}
