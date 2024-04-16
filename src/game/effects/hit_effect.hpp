#pragma once

#include <functional>
#include <entt/entity/fwd.hpp>

#include "extension/fmt.hpp"
#include "general/color.hpp"
#include "game/entities/team.hpp"

namespace spellbook {

struct Scene;

// Hit effects are a subset of effects
enum EffectType {
    EffectType_Custom,
    EffectType_Damage,
    EffectType_Heal,
    EffectType_Growth,
    EffectType_Regen,
    EffectType_Vulnerable,
    EffectType_Resistance,
    EffectType_Amplify,
    EffectType_Weaken,
    EffectType_Haste,
    EffectType_Slow,
    EffectType_Acid,
    EffectType_Ethereal,
    EffectType_Splash
};

struct EffectDatabase {
    struct EffectEntry {
        string name;
        string description;
        Color32 color;
        umap<string, string> extra_strings;
        umap<string, float> extra_floats;
    };
    bool initialized = false;
    vector<EffectEntry> entries;
};

void initialize_effect_database(EffectDatabase&);
EffectDatabase& get_effect_database();

struct HitEffect {
    EffectType type;
    int32 level = 1;
    float duration = 0.0f;
    float quantity = 1.0f;

    std::function<void(HitEffect&, Scene*, entt::entity, const vector<entt::entity>&)> apply_function;
    std::function<bool(Team::Disposition)> hits_disposition_function;

    HitEffect(EffectType type);
};

void effects_system(Scene* scene);

struct Acid {
    static constexpr float tick_delta = 0.5f;

    struct OnTickEffect {
        float response_end = 0.0f;
        std::function<void(Scene*, entt::entity)> response;
    };
    struct AcidLevel {
        entt::entity applier;
        int stacks;
        float effect_end = 0.0f;
        float next_tick = FLT_MAX;
    };
    std::array<AcidLevel, 4> levels;
    vector<OnTickEffect> tick_effects;
};


}