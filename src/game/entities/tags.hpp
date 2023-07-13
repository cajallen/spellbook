#pragma once

#include <entt/entity/fwd.hpp>

#include "general/umap.hpp"

namespace spellbook {

struct Scene;


struct Tags {
    struct Entry {
        uint32 tag;
        float until;

        bool has_dependency;
        entt::entity dependency;
    };

    Scene& scene;
    umap<uint64, Entry> entries;
    
    void apply_tag(uint32 tag, uint64 id, float duration = FLT_MAX);
    void apply_tag(uint32 tag, uint64 id, entt::entity dependency, float duration = FLT_MAX);
    void remove_tag(uint64 id);
    bool has_tag(uint32 tag);
};


}

