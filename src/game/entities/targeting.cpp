#include "targeting.hpp"

#include "spawner.hpp"
#include "game/scene.hpp"
#include "game/entities/ability.hpp"
#include "game/entities/components.hpp"
#include "game/entities/caster.hpp"

namespace spellbook {

bool taunted(Ability& ability, Caster& caster) {
    v3i target;
    if (caster.taunt.try_get(&target)) {
        ability.target = target;
        ability.has_target = true;
        return true;
    }
    return false;
}

bool square_targeting(int range, Ability& ability, EntryGatherFunction entry_gather) {
    v3i caster_pos = math::round_cast(ability.scene->registry.get<LogicTransform>(ability.caster).position);
    struct Entry {
        v3i offset = {};
        u32 count;
    };
    vector<Entry> entries;
    auto add_entry = [&ability, &entries, &caster_pos, &entry_gather](v3i offset) {
        float time_to = ability.time_to_hit(caster_pos + offset);
        entries.emplace_back(offset, (u32) entry_gather(ability, caster_pos + offset, time_to).size());
    };
    for (int x = -range; x <= range; x++) {
        for (int y = -range; y <= range; y++) {
            add_entry(v3i(x,y,0));
        }
    }
    
    if (entries.empty()) {
        ability.has_target = false;
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
        ability.target = caster_pos + closest_entries[math::random_s32(closest_entries.size())].offset;
        ability.has_target = true;
    } else {
        ability.has_target = false;
    }
    return ability.has_target;
}

bool plus_targeting(int range, Ability& ability, EntryGatherFunction entry_gather) {
    v3i caster_pos = math::round_cast(ability.scene->registry.get<LogicTransform>(ability.caster).position);
    struct Entry {
        v3i offset = {};
        u32 count;
    };
    vector<Entry> entries;
    auto add_entry = [&ability, &entries, &caster_pos, &entry_gather](v3i offset) {
        float time_to = ability.time_to_hit(caster_pos + offset);
        entries.emplace_back(offset, (u32) entry_gather(ability, caster_pos + offset, time_to).size());
    };

    for (const v2i& offset : {v2i{-range, 0}, v2i{0, -range}, v2i{range, 0}, v2i{0, range}}) {
        if (ability.scene->get_tile(caster_pos + v3i(offset.x, offset.y, -1)) == entt::null)
            continue;
        add_entry(v3i(offset.x, offset.y, 0)); 
    }

    if (entries.empty()) {
        ability.has_target = false;
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
        ability.target = caster_pos + closest_entries[math::random_s32(closest_entries.size())].offset;
        ability.has_target = true;
    } else {
        ability.has_target = false;
    }
    return ability.has_target;
}

bool end_targeting(Ability& ability) {
    // astar::Navigation& nav = *ability.scene->navigation;
    // entt::registry& registry = ability.scene->registry;
    // for (auto [entity, spawner, spawner_transform] : registry.view<Spawner, LogicTransform>().each()) {
    //     
    // }
    return false;
}


EntryGatherFunction square_aoe_entry_gather(int range) {
    return [range](Ability& ability, v3i pos, float time_to) -> uset<entt::entity> {
        uset<entt::entity> enemies;
        for (int x = -range; x <= range; x++) {
            for (int y = -range; y <= range; y++) {
                for (entt::entity enemy : ability.scene->targeting->select_enemies(pos + v3i(x, y, 0), time_to))
                    enemies.insert(enemy);
            }
        }
        return enemies;
    };
}

EntryGatherFunction enemy_entry_gather() {
    return [](Ability& ability, v3i pos, float time_to) -> uset<entt::entity> {
        entt::entity liz = ability.scene->targeting->select_lizard(pos);
        return liz == entt::null ? uset<entt::entity>{} : uset<entt::entity>{liz};
    };
}

EntryGatherFunction lizard_entry_gather() {
    return [](Ability& ability, v3i pos, float time_to) -> uset<entt::entity> {
        uset<entt::entity> enemies;
        for (entt::entity e : ability.scene->targeting->select_enemies(pos, time_to))
            enemies.insert(e);
        return enemies;
    };
}



}
