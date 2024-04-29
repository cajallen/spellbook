#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/signal/delegate.hpp>

#include "hit_effect.hpp"

namespace spellbook {

struct Scene;

struct HitReaction {
    uint64 optional_id;
    entt::delegate<void(Scene*, entt::entity /*proagator*/, entt::entity /*receiver*/, HitEffect& /*effect*/)> reaction;
    EffectType relies_on_effect_type;
    entt::entity relies_on_propagator;
    entt::entity relies_on_receiver;
    bool requires_special_activation = true;
};

struct HitSceneState {
    vector<HitReaction> reactions;

    void cleanup_on_death(entt::entity entity);
};

struct Hit {
    Scene* scene;
    v3i position;
    entt::entity caster;
    vector<HitEffect> effects;

    // internal
    umap<Team::Disposition, vector<entt::entity>> hit_entities;

    void do_reactions();
    void process();
    void pre_apply();
    void post_apply();
};

}