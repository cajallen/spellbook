#pragma once

#include <entt/entity/fwd.hpp>

#include "targeting.hpp"
#include "game/timer.hpp"

namespace spellbook {

struct Scene;
struct Caster;
struct AreaTrigger;

using AreaTriggerEntryGatherFunction = std::function<uset<entt::entity>(AreaTrigger&, v3i)>;

AreaTriggerEntryGatherFunction area_trigger_gather_lizard();
AreaTriggerEntryGatherFunction area_trigger_gather_enemies();

bool area_trigger_simple_targeting(AreaTrigger& trigger);

struct AreaTrigger {
    Scene* scene;
    entt::entity entity;
    entt::entity caster_entity;
    
    AreaTriggerEntryGatherFunction entry_gather;
    std::function<bool(AreaTrigger&)> targeting;
    std::function<void(AreaTrigger&)> trigger;

    bool triggered = false;
};

void area_trigger_system(Scene* scene);

}