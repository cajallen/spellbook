#include "targeting.hpp"

#include "consumer.hpp"
#include "enemy.hpp"
#include "spawner.hpp"
#include "general/astar.hpp"
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

bool square_targeting(int range, Ability& ability, const EntryGatherFunction& gather_func, const EntryEvalFunction& eval_func) {
    v3i caster_pos = math::round_cast(ability.scene->registry.get<LogicTransform>(ability.caster).position);
    struct Entry {
        v3i offset = {};
        int count;
    };
    vector<Entry> entries;
    auto add_entry = [&ability, &entries, &caster_pos, &gather_func, &eval_func](v3i offset) {
        float time_to = ability.time_to_hit(caster_pos + offset);
        int score = eval_func(ability.scene, gather_func(ability, caster_pos + offset, time_to));
        entries.emplace_back(offset, score);
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
        ability.target = caster_pos + closest_entries[math::random_int32(closest_entries.size())].offset;
        ability.has_target = true;
    } else {
        ability.has_target = false;
    }
    return ability.has_target;
}

bool plus_targeting(int range, Ability& ability, const EntryGatherFunction& gather_func, const EntryEvalFunction& eval_func) {
    v3i caster_pos = math::round_cast(ability.scene->registry.get<LogicTransform>(ability.caster).position);
    struct Entry {
        v3i offset = {};
        uint32 count;
    };
    vector<Entry> entries;
    auto add_entry = [&ability, &entries, &caster_pos, &gather_func, &eval_func](v3i offset) {
        float time_to = ability.time_to_hit(caster_pos + offset);
        int score = eval_func(ability.scene, gather_func(ability, caster_pos + offset, time_to));
        entries.emplace_back(offset, score);
    };

    for (const v2i& offset : {v2i{-range, 0}, v2i{0, -range}, v2i{range, 0}, v2i{0, range}}) {
        bool blocked = ability.scene->map_data.solids.get(caster_pos + v3i(offset.x, offset.y, 0));
        bool has_floor = ability.scene->map_data.solids.get(caster_pos + v3i(offset.x, offset.y, -1));
        if (blocked || !has_floor)
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
        ability.target = caster_pos + closest_entries[math::random_int32(closest_entries.size())].offset;
        ability.has_target = true;
    } else {
        ability.has_target = false;
    }
    return ability.has_target;
}

bool trap_targeting(Ability& ability) {
    LogicTransform& caster_tfm = ability.scene->registry.get<LogicTransform>(ability.caster);
    ability.has_target = false;
    for (int i = 0; true; i++) {
        bool any = false;
        for (auto& path_info : ability.scene->paths) {
            if (i >= path_info.path.waypoints.size())
                continue;
            any = true;
            
            if (math::abs(caster_tfm.position.z - path_info.path.waypoints[i].z) > 0.3f)
                continue;
                
            if (math::distance(path_info.path.waypoints[i], math::round(path_info.path.waypoints[i])) > 0.1f)
                continue;
            
            bool floor_occupied = false;
            for (entt::entity e : ability.scene->get_any(math::round_cast(path_info.path.waypoints[i]))) {
                if (ability.scene->registry.any_of<FloorOccupier>(e)) {
                    floor_occupied = true;
                    break;
                }
            }
            if (floor_occupied)
                continue;

            ability.target = math::round_cast(path_info.path.waypoints[i]);
            ability.has_target = true;
            return true;
        }
        if (!any)
            break;
    }
    return ability.has_target;
}


EntryGatherFunction gather_enemies_aoe(int range) {
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

EntryGatherFunction gather_enemies_floor() {
    return [](Ability& ability, v3i pos, float time_to) -> uset<entt::entity> {
        uset<entt::entity> enemies;
        for (auto [entity, enemy, logic_tfm] : ability.scene->registry.view<Enemy, LogicTransform>().each()) {
            int level_diff = math::round_cast(logic_tfm.position.z) - pos.z;
            if (level_diff == 0)
                enemies.insert(entity);
        }
        return enemies;
    };
}

EntryGatherFunction gather_lizard() {
    return [](Ability& ability, v3i pos, float time_to) -> uset<entt::entity> {
        entt::entity liz = ability.scene->targeting->select_lizard(pos);
        return liz == entt::null ? uset<entt::entity>{} : uset<entt::entity>{liz};
    };
}

EntryGatherFunction gather_enemies() {
    return [](Ability& ability, v3i pos, float time_to) -> uset<entt::entity> {
        uset<entt::entity> enemies;
        for (entt::entity e : ability.scene->targeting->select_enemies(pos, time_to))
            enemies.insert(e);
        return enemies;
    };
}

int simple_entry_eval(Scene* scene, const uset<entt::entity>& units) {
        return units.size();
}

int basic_lizard_entry_eval(Scene* scene, const uset<entt::entity>& units) {
    int counter = 0;
    for (entt::entity entity : units) {
        entt::entity attachment_entity = scene->registry.get<Enemy>(entity).attachment;
        if (scene->registry.any_of<Egg>(attachment_entity))
            counter += 10;
        else
            counter += 1;
    }
    return counter;
}



}
