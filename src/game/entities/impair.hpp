#pragma once

#include <entt/entity/fwd.hpp>

#include "general/umap.hpp"

namespace spellbook {

struct Scene;

enum ImpairType : u8 {
    ImpairType_None,
    ImpairType_NoMove,
    ImpairType_NoCast
};

struct Impairs {
    struct TimedEntry {
        ImpairType type;
        float until;
    };
    umap<u64, ImpairType> untimed_impairs;
    umap<u64, TimedEntry> timed_impairs;

    bool is_impaired(Scene* scene, ImpairType type);
};

void apply_untimed_impair(Impairs& impairs, u64 id, ImpairType type);
void apply_timed_impair(Scene* scene, entt::entity entity, u64 id, ImpairType type, float time);

}

