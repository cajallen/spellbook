﻿#pragma once

#include <entt/entity/fwd.hpp>

#include "general/math/geometry.hpp"
#include "general/color.hpp"
#include "general/file/resource.hpp"

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

struct Beads {
    Bead type;
    int32 amount;
};

struct BeadPrefab : Resource {
    FilePath model_path;
    Bead type;
    float scale = 1.0f;

    static constexpr string_view extension() { return ".sbjdrp"; }
    static constexpr string_view dnd_key() { return "DND_DROP"; }
    static FilePath folder() { return "drops"_resource; }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == BeadPrefab::extension(); }; }
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