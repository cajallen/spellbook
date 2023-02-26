#pragma once

#include <entt/entity/fwd.hpp>

#include "general/string.hpp"
#include "general/geometry.hpp"
#include "general/color.hpp"

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
    string file_path;
    string model_path;
    Bead type;
    float scale = 1.0f;
};

struct DropChance {
    struct Entry {
        string bead_prefab_path;
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

}