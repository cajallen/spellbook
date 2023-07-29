#pragma once

#include <entt/entity/fwd.hpp>

#include "general/math/geometry.hpp"
#include "general/color.hpp"
#include "general/file_path.hpp"

namespace spellbook {

struct Scene;

enum Bead {
    Bead_Oak,
    Bead_Yew,
    Bead_Amber,
    Bead_Malachite,
    Bead_Quartz,
    Bead_Count
};

struct BeadPrefab {
    FilePath file_path;
    vector<FilePath> dependencies;
    
    FilePath model_path;
    Bead type;
    float scale = 1.0f;
};

struct DropChance {
    struct Entry {
        FilePath bead_prefab_path;
        float drop_chance = 1.0f;
    };

    vector<Entry> entries;
};

struct Pickup {
    // Could be replaced with a callback
    Bead bead_type;
    float cycle_point;
};

entt::entity instance_prefab(Scene* scene, const BeadPrefab& bead_prefab, v3 position);
bool inspect(BeadPrefab* bead_prefab);
bool inspect(DropChance::Entry* drop_chance_entry);
bool inspect(DropChance* drop_chance);

Color bead_color(Bead bead);

JSON_IMPL(BeadPrefab, model_path, type, scale);
JSON_IMPL(DropChance::Entry, bead_prefab_path, drop_chance);
JSON_IMPL(DropChance, entries);

}