#pragma once

#include <entt/entity/entity.hpp>

#include "general/geometry.hpp"
#include "general/umap.hpp"

namespace spellbook {

struct Ability;
struct Caster;

using EntryGatherFunction = std::function<uset<entt::entity>(Ability&, v3i, float)>;


bool square_targeting(int range, Ability& ability, EntryGatherFunction entry_eval);
bool plus_targeting(int range, Ability& ability, EntryGatherFunction entry_eval);

bool taunted(Ability&, Caster&);

EntryGatherFunction square_aoe_entry_gather(int range);
EntryGatherFunction enemy_entry_gather();
EntryGatherFunction lizard_entry_gather();

}
