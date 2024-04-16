#pragma once

#include "hit_effect.hpp"

namespace spellbook {


struct Hit {
    Scene* scene;
    v3i position;
    entt::entity caster;
    vector<HitEffect> effects;

    void process();
    void pre_apply();
    void post_apply();
};

}