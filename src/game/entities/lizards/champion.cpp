#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "general/matrix_math.hpp"
#include "renderer/draw_functions.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizard.hpp"

namespace spellbook {

void champion_attack_start(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);

    v3i caster_pos = math::round_cast(ability->scene->registry.get<LogicTransform>(ability->caster).position);
    
    auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
    if (poser) {
        poser->set_state(AnimationState_AttackInto, ability->pre_trigger_time.value());
    }
    auto lizard = ability->scene->registry.try_get<Lizard>(ability->caster);
    if (lizard) {
        v3 dir_to = math::normalize(v3(ability->target) - v3(caster_pos));
        float ang = math::angle_difference(lizard->default_direction.xy, dir_to.xy);
        ability->scene->registry.get<LogicTransform>(ability->caster).rotation.yaw = ang;
    }
}

void champion_attack_trigger(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);
    
    uset<entt::entity> enemies;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (entt::entity enemy : ability->scene->get_enemies(ability->target + v3i(x, y, 0)))
                enemies.insert(enemy);
        }
    }
    
    for (auto& enemy : enemies) {
        auto& health = ability->scene->registry.get<Health>(enemy);
        auto& this_lt = ability->scene->registry.get<LogicTransform>(ability->caster);
        auto& enemy_lt = ability->scene->registry.get<LogicTransform>(enemy);
        health.damage(2.0f, enemy_lt.position - this_lt.position);
    }
    auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
    if (poser) {
        poser->set_state(AnimationState_AttackOut, ability->post_trigger_time.value());
    }

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            entt::entity tile = ability->scene->get_tile(ability->target + v3i(x, y, -1));
            if (tile == entt::null) {
                quick_emitter(ability->scene, "Champion Basic", v3(ability->target + v3i(x, y, 0)), "emitters/champion_basic_fizzle.sbemt", 0.20f);
                continue;
            }

            auto& grid_slot = ability->scene->registry.get<GridSlot>(tile);
            if ((grid_slot.path || grid_slot.ramp) && x == 0 && y == 0) {
                quick_emitter(ability->scene, "Champion Basic", v3(ability->target + v3i(x, y, 0)), "emitters/champion_basic_hit.sbemt", 0.20f);
            } else {
                quick_emitter(ability->scene, "Champion Basic", v3(ability->target + v3i(x, y, 0)), "emitters/champion_basic_miss.sbemt", 0.20f);
            }
        }
    }
}

void champion_attack_end(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);
    auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
    if (poser) {
        poser->set_state(AnimationState_Idle);
    }
}

void champion_attack_targeting(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);
    v3i caster_pos = math::round_cast(ability->scene->registry.get<LogicTransform>(ability->caster).position);
    struct Entry {
        v3i offset = {};
        int count;
    };
    vector<Entry> entries;
    auto add_entry = [&ability, &entries, &caster_pos](v3i offset) {
        uset<entt::entity> enemies;
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                for (entt::entity enemy : ability->scene->get_enemies(caster_pos + offset + v3i(x, y, 0)))
                    enemies.insert(enemy);
            }
        }
        entries.emplace_back(offset, int(enemies.size()));
    };

    for (const v2i& offset : {v2i{-2, 0}, v2i{0, -2}, v2i{2, 0}, v2i{0, 2}}) {
        if (ability->scene->get_tile(caster_pos + v3i(offset.x, offset.y, -1)) == entt::null)
            continue;
        add_entry(v3i(offset.x, offset.y, 0)); 
    }

    if (entries.empty()) {
        ability->has_target = false;
        return;
    }
    
    vector closest_entries = {entries.front()};
    for (auto& entry : entries) {
        if (entry.count > closest_entries.begin()->count)
            closest_entries = {entry};
        else if (entry.count == closest_entries.begin()->count)
            closest_entries.push_back(entry);
    }
    if (closest_entries.front().count > 0) {
        ability->target = caster_pos + closest_entries[math::random_s32(closest_entries.size())].offset;
        ability->has_target = true;
    } else {
        ability->has_target = false;
    }
}

void build_champion(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    auto& liz = scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    
    liz.basic_ability = make_ability(scene, "Champion Basic");
    liz.basic_ability->caster = entity;
    liz.basic_ability->pre_trigger_time = Stat(1.2f);
    liz.basic_ability->post_trigger_time = Stat(1.0f);
    liz.basic_ability->cooldown_time = Stat(1.2f);
    liz.basic_ability->start_callback = champion_attack_start;
    liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->trigger_callback = champion_attack_trigger;
    liz.basic_ability->trigger_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->end_callback = champion_attack_end;
    liz.basic_ability->end_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->targeting_callback = champion_attack_targeting;
    liz.basic_ability->targeting_payload = (void*) liz.basic_ability.id;
}

void draw_champion_dragging_preview(Scene* scene, entt::entity entity) {
    v3 logic_pos = scene->registry.get<Dragging>(entity).potential_logic_position;
    v3i logic_posi = math::round_cast(logic_pos);
    
    vector<FormattedVertex> vertices;
    for (const v2i& offset : {v2i{-2, 0}, v2i{0, -2}, v2i{2, 0}, v2i{0, 2}}) {
        if (scene->get_tile(logic_posi + v3i(offset.x, offset.y, -1)) != entt::null) {
            v3 pos = logic_pos + v3(0.5f, 0.5f, 0.02f) + v3(offset.x, offset.y, 0.0f);
            add_formatted_square(vertices, pos, v3(0.5f, 0.f, 0.f), v3(0.f, 0.5f, 0.f), palette::aquamarine, 0.05f);
            add_formatted_square(vertices, pos, v3(1.5f, 0.f, 0.f), v3(0.f, 1.5f, 0.f), palette::dark_sea_green, 0.03f);
        }
    }
    if (vertices.empty())
        return;
    scene->render_scene.quick_mesh(generate_formatted_line(&scene->camera, vertices), true, false);
}

}
