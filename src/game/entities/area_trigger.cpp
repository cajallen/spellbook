#include "area_trigger.hpp"

#include <tracy/Tracy.hpp>

#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/ability.hpp"

namespace spellbook {


AreaTriggerEntryGatherFunction area_trigger_gather_lizard() {
    return [](AreaTrigger& ability, v3i pos) -> uset<entt::entity> {
        entt::entity liz = ability.scene->targeting->select_lizard(pos);
        return liz == entt::null ? uset<entt::entity>{} : uset<entt::entity>{liz};
    };
}

AreaTriggerEntryGatherFunction area_trigger_gather_enemies() {
    return [](AreaTrigger& ability, v3i pos) -> uset<entt::entity> {
        uset<entt::entity> enemies;
        for (entt::entity e : ability.scene->targeting->select_enemies(pos, 0.0f))
            enemies.insert(e);
        return enemies;
    };
}

bool area_trigger_simple_targeting(AreaTrigger& area_trigger) {
    v3i trap_pos = math::round_cast(area_trigger.scene->registry.get<LogicTransform>(area_trigger.entity).position);
    struct Entry {
        v3i offset = {};
        uint32 count;
    };
    vector<Entry> entries;
    entries.emplace_back(v3i(0), (uint32) area_trigger.entry_gather(area_trigger, trap_pos).size());

    if (entries.empty()) {
        return false;
    }

    vector closest_entries = {entries.front()};
    for (auto& entry : entries) {
        if (entry.count > closest_entries.begin()->count)
            closest_entries = {entry};
        else if (entry.count == closest_entries.begin()->count)
            closest_entries.push_back(entry);
    }
    if (closest_entries.front().count > 0) {
        return true;
    }
    return false;
}

void area_trigger_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, l_transform, area_trigger] : scene->registry.view<LogicTransform, AreaTrigger>().each()) {
        if (!area_trigger.triggered && area_trigger.targeting(area_trigger)) {
            area_trigger.triggered = true;
            area_trigger.trigger(area_trigger);
        }
    }
}

}