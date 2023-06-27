#pragma once

#include <entt/entity/entity.hpp>

#include "general/geometry.hpp"
#include "general/umap.hpp"

namespace spellbook {

struct Ability;
struct Caster;
struct Scene;

using EntryEvalFunction = std::function<int(Scene*, const uset<entt::entity>&)>;
using EntryGatherFunction = std::function<uset<entt::entity>(Ability&, v3i, float)>;

bool trap_targeting(Ability& ability);

bool square_targeting(int range, Ability& ability, const EntryGatherFunction& gather_func, const EntryEvalFunction& eval_func);
bool plus_targeting(int range, Ability& ability, const EntryGatherFunction& gather_func, const EntryEvalFunction& eval_func);

bool taunted(Ability&, Caster&);

int simple_entry_eval(Scene* scene, const uset<entt::entity>& units);
int basic_lizard_entry_eval(Scene* scene, const uset<entt::entity>& units);
EntryGatherFunction gather_enemies_aoe(int range);
EntryGatherFunction gather_enemies_floor();
EntryGatherFunction gather_lizard();
EntryGatherFunction gather_enemies();

}
