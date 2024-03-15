#pragma once

#include <entt/entity/entity.hpp>

#include "extension/fmt.hpp"
#include "general/color.hpp"
#include "general/file/file_path.hpp"
#include "game/entities/ability.hpp"

namespace spellbook {

struct Scene;

enum LizardType {
    LizardType_Empty,
    LizardType_Barbarian,
    LizardType_Champion,
    LizardType_Assassin,
    LizardType_Ranger,
    LizardType_IllusionMage,
    LizardType_MindMage,
    LizardType_WaterMage,
    LizardType_Warlock,
    LizardType_Thief,
    LizardType_Druid,
    LizardType_LaserMage,
    LizardType_Treasurer,
    LizardType_SacrificeSupport,
    LizardType_Bulwark
};

struct Lizard { // Component
    LizardType type = LizardType_Empty;
    v3 default_direction = v3(0,1,0);
    std::function<void(Scene*, entt::entity)> dragging_preview_function;
};

template <LizardType T>
void draw_dragging_preview(Scene* scene, entt::entity entity);

constexpr uint32 ability_index(LizardType lt, AbilityType at) { return lt * magic_enum::enum_count<AbilityType>() + at; }
struct LizardDatabase {
    struct LizardEntry {
        LizardType type;
        string name;
        Color32 color;
        float scale;
        FilePath model_path;
        FilePath hurt_emitter;
        umap<string, string> extra_strings;
        umap<string, float> extra_floats;
    };

    struct AbilityEntry {
        LizardType lizard_type;
        AbilityType ability_type;
        string name;
        string description;
        float pre_time;
        float post_time;
        umap<string, string> extra_strings;
        umap<string, float> extra_floats;
    };


    bool initialized = false;
    vector<LizardEntry> lizard_entries;
    vector<AbilityEntry> ability_entries;
};
void initialize_lizard_database(LizardDatabase&);
LizardDatabase& get_lizard_database();

template <LizardType T>
void build(Scene* scene, entt::entity entity);

}