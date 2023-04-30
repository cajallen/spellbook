#include "enemy.hpp"

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <imgui.h>

#include "caster.hpp"
#include "impair.hpp"
#include "targeting.hpp"
#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/astar.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/consumer.hpp"
#include "renderer/draw_functions.hpp"


namespace spellbook {

struct EnemyLaserAttack : Ability {
    void targeting() override;
    void trigger() override;
};

void enemy_fallback_targeting(Ability& ability) {
    auto consumers = ability.scene->registry.view<Consumer>();
    if (consumers.empty())
        return;
    entt::entity consumer_entity = consumers[math::random_s32(consumers.size())];
    LogicTransform& consumer_transform = ability.scene->registry.get<LogicTransform>(consumer_entity);
    ability.target = math::round_cast(consumer_transform.position);
    ability.has_target = true;
}

void EnemyLaserAttack::trigger() {
    uset<entt::entity> lizards = entry_gather_function(*this, target, 0.0f);
    for (entt::entity lizard : lizards) {
        auto& this_lt = scene->registry.get<LogicTransform>(caster);
        auto& liz_lt = scene->registry.get<LogicTransform>(lizard);
        
        Health& health = scene->registry.get<Health>(lizard);
        damage(scene, caster, lizard, 1.0f, liz_lt.position - this_lt.position);

        constexpr float duration = 0.15f;
        entt::entity caster_v = caster;
        add_tween_timer(scene, "Enemy laser tick", [this, lizard, caster_v](Timer* timer) {
            if (timer->ticking && timer->scene->registry.valid(lizard) && timer->scene->registry.valid(caster_v)) {
                auto* this_lt = scene->registry.try_get<LogicTransform>(caster);
                auto* liz_lt = scene->registry.try_get<LogicTransform>(lizard);
                if (this_lt && liz_lt) {
                    vector<FormattedVertex> vertices;
                    float width = (timer->remaining_time / timer->total_time) * 0.10f + 0.05f;
                    vertices.emplace_back(this_lt->position + v3(0.5f), palette::light_pink, width + 0.05f);
                    vertices.emplace_back(liz_lt->position + v3(0.5f), palette::red, width);
                    scene->render_scene.quick_mesh(generate_formatted_line(&scene->camera, vertices), true, false);
                }
            }
        }, false).start(duration);
    }
}


void EnemyLaserAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);

    if (taunted(*this, caster_comp))
        return;

    if (square_targeting(1, *this, entry_gather_function))
        return;

    enemy_fallback_targeting(*this);
}

entt::entity instance_prefab(Scene* scene, const EnemyPrefab& prefab, v3i location) {
    auto entity = setup_basic_unit(scene, prefab.model_path, v3(location), prefab.max_health, prefab.hurt_path);

    scene->registry.emplace<Enemy>(entity);
    auto& traveler = scene->registry.emplace<Traveler>(entity, vector<v3i>{});
    traveler.max_speed = Stat(scene, prefab.max_speed);
    traveler.range = Stat(scene, 1.01f);

    scene->registry.get<ModelTransform>(entity).scale = v3(prefab.scale);
    scene->registry.get<TransformLink>(entity).offset.z = 0.5f;

    if (!prefab.drops.entries.empty())
        scene->registry.emplace<DropChance>(entity, prefab.drops);
    
    Caster& caster = scene->registry.get<Caster>(entity);

    switch (prefab.type) {
        default: {
            //  TODO: need cooldown to avoid constant movement... aggro immunity maybe?
            caster.attack = std::make_unique<EnemyLaserAttack>();
            caster.attack->setup(scene, entity, 0.5f, 1.0f, Ability::Type_Attack);
            caster.attack->entry_gather_function = enemy_entry_gather();
        } break;
    }
    
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
    for (auto [entity, transform, traveler, caster, impair] : scene->registry.view<LogicTransform, Traveler, Caster, Impairs>().each()) {
        if (!caster.attack->has_target)
            continue;
        
        float d = math::distance(v3(caster.attack->target), transform.position);
        // Update target
        if (traveler.pathing.empty() || traveler.pathing.front() != caster.attack->target) {
            if (d > traveler.range.value()) {
                if (caster.attack->casting())
                    caster.attack->stop_casting();

                traveler.pathing = scene->navigation->find_path(math::round_cast(transform.position), caster.attack->target);
                if (traveler.pathing.empty())
                    continue;
            }
        }

        if (d < traveler.range.value()) {
            apply_untimed_impair(impair, 0x724f3132, ImpairType_None);
        } else {
            apply_untimed_impair(impair, 0x724f3132, ImpairType_NoCast);
        }

        if (!caster.attack->casting() && !traveler.pathing.empty() && d > traveler.range.value()) {
            v3   velocity        = v3(traveler.pathing.back()) - transform.position;
            bool at_target       = math::length(velocity) < 0.01f;
            if (at_target) {
                traveler.pathing.remove_back();
                velocity = v3(0);
            }
            f32 max_velocity = traveler.max_speed.value() * scene->delta_time;
            f32 min_velocity = 0.0f;
            if (!at_target)
                transform.position += math::normalize(velocity) * math::clamp(math::length(velocity), min_velocity, max_velocity);
        }
    }
}

v3 predict_pos(Traveler& traveler, v3 pos, float time) {
    v3 expected_pos = pos;
    if (time == 0.0f)
        return expected_pos;
    for (u32 i = traveler.pathing.size(); i > 0; i--) {
        if (math::distance(v3(traveler.pathing[0]), expected_pos) < traveler.range.value()) {
            return expected_pos;
        }
        v3 pos1 = i == traveler.pathing.size() ? pos : v3(traveler.pathing[i]);
        v3 pos2 = v3(traveler.pathing[i-1]);
        float time_delta = math::distance(v3(pos1), v3(pos2)) / traveler.max_speed.value();
        if (time_delta > time) {
            expected_pos = math::lerp(time / time_delta, range3{v3(pos1), v3(pos2)});
            break;
        }
        time -= time_delta;
        expected_pos = pos2;
    }
    return expected_pos;
}


}
