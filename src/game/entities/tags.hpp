#pragma once

#include <entt/entity/fwd.hpp>

#include "general/umap.hpp"

namespace spellbook {

struct Scene;


struct Tags {
    struct Entry {
        u32 tag;
        float until;

        bool has_dependency;
        entt::entity dependency;
    };

    Scene& scene;
    umap<u64, Entry> entries;
    
    void apply_tag(u32 tag, u64 id, float duration = FLT_MAX);
    void apply_tag(u32 tag, u64 id, entt::entity dependency, float duration = FLT_MAX);
    void remove_tag(u64 id);
    bool has_tag(u32 tag);
};


}

