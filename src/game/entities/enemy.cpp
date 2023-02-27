#include "enemy.hpp"

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <imgui.h>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/astar.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/consumer.hpp"
#include "renderer/draw_functions.hpp"


namespace spellbook {

void enemy_attack_start(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);
}

void enemy_attack_trigger(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);

    uset<entt::entity> lizards;
    entt::entity lizard = ability->scene->get_lizard(ability->target);
    if (lizard != entt::null) {
        auto& this_lt = ability->scene->registry.get<LogicTransform>(ability->caster);
        auto& liz_lt = ability->scene->registry.get<LogicTransform>(lizard);
        
        Health& health = ability->scene->registry.get<Health>(lizard);
        health.damage(1.0f, liz_lt.position - this_lt.position);

        struct EnemyLaserPayload {
            Scene* scene;
            entt::entity enemy;
            entt::entity liz;
        };

        constexpr float duration = 0.15f;
        add_tween_timer(ability->scene, "Enemy laser tick", [](Timer* timer, void* data) {
            auto payload = (EnemyLaserPayload*) data;

            if (timer->ticking && payload->scene->registry.valid(payload->enemy) && payload->scene->registry.valid(payload->liz)) {
                auto* this_lt = payload->scene->registry.try_get<LogicTransform>(payload->enemy);
                auto* liz_lt = payload->scene->registry.try_get<LogicTransform>(payload->liz);
                if (this_lt && liz_lt) {
                    vector<FormattedVertex> vertices;
                    float width = (timer->remaining_time / timer->total_time) * 0.10f + 0.05f;
                    vertices.emplace_back(this_lt->position + v3(0.5f), palette::light_pink, width + 0.05f);
                    vertices.emplace_back(liz_lt->position + v3(0.5f), palette::red, width);
                    payload->scene->render_scene.quick_mesh(generate_formatted_line(&payload->scene->camera, vertices), true, false);
                }
            }
        }, new EnemyLaserPayload{ability->scene, ability->caster, lizard}, true, false).start(duration);
    }
}

void enemy_attack_end(void* payload) {
}

void enemy_attack_targeting(void* payload) {
}

entt::entity instance_prefab(Scene* scene, const EnemyPrefab& prefab, v3i location) {
    static int i      = 0;
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", fs::path(prefab.file_path).stem().string(), i++));
    
    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(prefab.model_path));
    model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);

    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity, v3{}, euler{}, v3(prefab.scale));
    scene->registry.emplace<TransformLink>(entity, v3(0.5));

    auto& enemy = scene->registry.emplace<Enemy>(entity, vector<v3i>{}, prefab.max_speed);
    scene->registry.emplace<Health>(entity, prefab.max_health, &scene->render_scene, prefab.hurt_path);
    if (!prefab.drops.entries.empty())
        scene->registry.emplace<DropChance>(entity, prefab.drops);


    enemy.ability = make_ability(scene, "Enemy Attack");
    enemy.ability->caster = entity;
    enemy.ability->pre_trigger_time = Stat(0.5f);
    enemy.ability->post_trigger_time = Stat(0.5f);
    enemy.ability->cooldown_time = Stat(1.0f);
    enemy.ability->start_callback = enemy_attack_start;
    enemy.ability->start_payload = (void*) enemy.ability.id;
    enemy.ability->trigger_callback = enemy_attack_trigger;
    enemy.ability->trigger_payload = (void*) enemy.ability.id;
    enemy.ability->end_callback = enemy_attack_end;
    enemy.ability->end_payload = (void*) enemy.ability.id;
    enemy.ability->targeting_callback = enemy_attack_targeting;
    enemy.ability->targeting_payload = (void*) enemy.ability.id;
    
    return entity;  
}

bool inspect(EnemyPrefab* enemy_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &enemy_prefab->file_path, "resources/enemies", FileType_Enemy);
    changed |= ImGui::EnumCombo("Type", &enemy_prefab->type);
    changed |= ImGui::PathSelect("Model", &enemy_prefab->model_path, "resources/models", FileType_Model);
    changed |= ImGui::PathSelect("Hurt", &enemy_prefab->hurt_path, "resources/emitters", FileType_Emitter);
    changed |= ImGui::DragFloat("Max Health", &enemy_prefab->max_health, 0.01f, 0.0f);
    changed |= ImGui::DragFloat("Max Speed", &enemy_prefab->max_speed, 0.01f, 0.0f);
    changed |= ImGui::DragFloat("Scale", &enemy_prefab->scale, 0.01f, 0.0f);
    changed |= inspect(&enemy_prefab->drops);
    
    return changed;
}

void travel_system(Scene* scene) {
    // construct grid to use for pathfinding
    auto slots     = scene->registry.view<GridSlot, LogicTransform>();
    auto entities  = scene->registry.view<Enemy, LogicTransform>();
    auto consumers = scene->registry.view<Consumer>();

    astar::Navigation nav;
    for (auto [entity, slot, logic_pos] : slots.each()) {
        if (slot.ramp) {
            nav.ramps[v3i(logic_pos.position)] = slot.direction;
            continue;
        }
        if (slot.path)
            nav.solids.set(v3i(logic_pos.position));
    }
    
    // handle actual traveling
    for (auto [entity, enemy, transform] : entities.each()) {
        if (enemy.ability->casting())
            continue;

        bool has_aggro = false;
        v3 aggro;
        if (enemy.ability->ready_to_cast()) {
            if (enemy.taunt.first != 0) {
                has_aggro = true;
                auto l_transform = scene->registry.try_get<LogicTransform>(enemy.taunt.second);
                if (l_transform)
                    aggro = l_transform->position;
                else {
                    enemy.taunt = {0, entt::null};
                    has_aggro = false;
                }
            }
            if (enemy.position_target.first) {
                entt::entity lizard_at_target = scene->get_lizard(enemy.position_target.second);
                if (lizard_at_target != entt::null) {
                    has_aggro = true;
                    aggro = v3(enemy.position_target.second);
                } else {
                    enemy.position_target = {false, v3i{}};
                }
            }
            
            if (has_aggro && math::distance(aggro, transform.position) < 1.5f) {
                enemy.ability->target = math::round_cast(aggro);
                enemy.ability->request_cast();
                continue;
            }
        }

        bool has_path = !enemy.pathing.empty();

        if (!has_path && !consumers.empty()) {
            int       random_consumer = math::random_s32() % s32(consumers.size());
            auto      consumer_entity = consumers[random_consumer];
            LogicTransform* consumer_position          = scene->registry.try_get<LogicTransform>(consumer_entity);
            assert_else(consumer_position);

            auto path = nav.find_path(math::round_cast(transform.position), v3i(consumer_position->position));
            for (auto it = path.begin(); it != path.end(); ++it) {
                enemy.pathing.push_back(*it);
            }
        }
        assert_else(!math::is_nan(transform.position.x)) {
            transform.position = v3(0, 0, 0);
        }

        v3i  target_position = has_path ? enemy.pathing.back() : math::round_cast(transform.position);
        if (has_aggro) {
            // If we have aggro, set it as target
            target_position = math::round_cast(aggro);
        } else {
            // If we don't, check if we should
            entt::entity lizard_at_target = scene->get_lizard(target_position);
            if (lizard_at_target != entt::null) {
                enemy.position_target = {true, target_position};
                continue;
            }
        }

        v3   velocity        = v3(target_position) - transform.position;
        bool at_target       = math::length(velocity) < 0.01f;
        if (at_target && has_path) {
            enemy.pathing.remove_back();
            velocity = v3(0);
        }
        f32 max_velocity = enemy.max_speed.value() * scene->delta_time;
        f32 min_velocity = 0.0f;
        if (!at_target)
            transform.position += math::normalize(velocity) * math::clamp(math::length(velocity), min_velocity, max_velocity);
    }
}



}
