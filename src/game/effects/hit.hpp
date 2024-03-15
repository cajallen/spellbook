#pragma once

#include "hit_effect.hpp"

namespace spellbook {


struct Hit {
    Scene* scene;
    v3i location;
    entt::entity caster;
    vector<HitEffect> effects;

    void process();
};

}