#pragma once

#include <entt/entity/fwd.hpp>

#include "general/function.hpp"
#include "game/timer.hpp"
#include "game/entities/targeting.hpp"

namespace spellbook {

struct Scene;
struct Caster;
struct AreaTrigger;

using AreaTriggerEntryGatherFunction = function<uset<entt::entity>(AreaTrigger&, v3i)>;

AreaTriggerEntryGatherFunction area_trigger_gather_lizard();
AreaTriggerEntryGatherFunction area_trigger_gather_enemies();

bool area_trigger_simple_targeting(AreaTrigger& trigger);

struct AreaTrigger {
    Scene* scene;
    entt::entity entity;
    entt::entity caster_entity;
    
    AreaTriggerEntryGatherFunction entry_gather;
    function<bool(AreaTrigger&)> targeting;
    function<void(AreaTrigger&)> trigger;

    bool triggered = false;
};

void area_trigger_system(Scene* scene);

}