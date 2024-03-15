#pragma once

#include <entt/entity/fwd.hpp>

#include "extension/fmt.hpp"
#include "general/color.hpp"
#include "game/entities/team.hpp"

namespace spellbook {

struct Scene;

struct EffectDatabase {
    struct EffectEntry {
        string name;
        string description;
        Color32 color;
        umap<string, string> extra_strings;
        umap<string, float> extra_floats;
    };
    bool initialized = false;
    umap<uint64, EffectEntry> entries;
};

void initialize_effect_database(EffectDatabase&);
EffectDatabase& get_effect_database();

struct HitEffect {
    int32 level = 1;
    float duration = 0.0f;
    float quantity = 1.0f;

    virtual void apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) {}
    virtual bool hits_disposition(Team::Disposition disposition) { return false; }
};

struct Damage : HitEffect {
    void apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) override;
    bool hits_disposition(Team::Disposition disposition) override;
};

struct Vulnerable : HitEffect {
    static constexpr float effect[] = {0.3f, 0.5f, 1.0f};
    void apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) override;
    bool hits_disposition(Team::Disposition disposition) override;
};

struct Resistance : HitEffect {
    static constexpr float effect[] = {0.2f, 0.5f, 0.9f};
    void apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) override;
    bool hits_disposition(Team::Disposition disposition) override;
};

struct Slow : HitEffect {
    static constexpr float effect[] = {0.2f, 0.4f, 0.7f, 1.0f};
    void apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) override;
    bool hits_disposition(Team::Disposition disposition) override;
};

struct Haste : HitEffect {
    static constexpr float effect[] = {0.2f, 0.5f};
    void apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) override;
    bool hits_disposition(Team::Disposition disposition) override;
};

struct Poison : HitEffect {
    static constexpr float effect[] = {1.0f, 2.0f, 4.0f, 8.0f};
    void apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) override;
    bool hits_disposition(Team::Disposition disposition) override;
};

struct Growth : HitEffect {
    void apply(Scene* scene, entt::entity caster, const vector<entt::entity>& targets) override;
    bool hits_disposition(Team::Disposition disposition) override;
};

}