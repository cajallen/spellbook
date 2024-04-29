#pragma once

#include <entt/entity/entity.hpp>

#include "extension/fmt.hpp"
#include "general/color.hpp"
#include "general/file/file_path.hpp"
#include "game/entities/ability.hpp"

#define LIZARD_VARIANT_COUNT 4

namespace spellbook {

struct Scene;

enum LizardType {
    LizardType_Empty,
    LizardType_Hanther,
    LizardType_Sauria,
    LizardType_Rokko,
    LizardType_Empyrea
};

struct Lizard { // Component
    LizardType type = LizardType_Empty;
    uint8 lizard_variant = 0;
    std::function<void(Scene*, entt::entity)> dragging_preview_function;
};

template <LizardType T, uint8 Variant>
void draw_dragging_preview(Scene* scene, entt::entity entity);

constexpr uint32 ability_index(LizardType lizard_type, uint8 variant, AbilityType ability_type) {
    return
        lizard_type * (magic_enum::enum_count<AbilityType>() + LIZARD_VARIANT_COUNT)
        + variant * LIZARD_VARIANT_COUNT
        + ability_type;
}
struct LizardDatabase {
    struct LizardEntry {
        struct VariantInfo {
            string title;
            FilePath splash;
            Color32 color;
            uint8 uv = 0;
        };
        LizardType type;
        std::array<VariantInfo, LIZARD_VARIANT_COUNT> variants;
        string name;
        float scale;
        FilePath model_path;
        FilePath hurt_emitter;
        FilePath background;
        umap<string, string> extra_strings;
        umap<string, float> extra_floats;
    };

    struct AbilityEntry {
        LizardType lizard_type;
        uint8 lizard_variant;
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

entt::entity instance_lizard(Scene* scene, v3i position, LizardType lizard_type, uint8 lizard_variant, std::function<void(Scene*, entt::entity)> build_func);

template <LizardType T, uint8 Variant>
void build(Scene* scene, entt::entity entity);

}