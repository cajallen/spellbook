#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/targeting.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

constexpr int poison_max_stacks = 4;
constexpr float poison_dps = 0.3f;
constexpr float poison_duration = 4.0f;

struct AssassinAttack : Ability {
    int phase = 0;
    void targeting() override;
    void start() override;
    void trigger() override;
    void end() override;
};

void AssassinAttack::start() {
    auto poser = scene->registry.try_get<PoseController>(caster);
    if (poser) {
        if (phase == 0) {
            poser->set_state(AnimationState_AttackInto, pre_trigger_time.value());
        } else {
            poser->set_state(AnimationState_Attack2Into, pre_trigger_time.value());
        }
    }

    v3i caster_pos = math::round_cast(scene->registry.get<LogicTransform>(caster).position);
    
    auto lizard = scene->registry.try_get<Lizard>(caster);
    if (lizard) {
        v3 dir_to = math::normalize(v3(target) - v3(caster_pos));
        float ang = math::angle_difference(lizard->default_direction.xy, dir_to.xy);
        scene->registry.get<LogicTransform>(caster).rotation.yaw = ang;
    }
}

void AssassinAttack::trigger() {
    auto& this_lt = scene->registry.get<LogicTransform>(caster);
    v3 hit_vec = v3(target) - this_lt.position;
    
    uset<entt::entity> enemies = entry_gather_function(*this, target, 0.0f);
    for (auto& enemy : enemies) {
        auto& health = scene->registry.get<Health>(enemy);
        auto& enemy_lt = scene->registry.get<LogicTransform>(enemy);
        damage(scene, caster, enemy, 3.0f / enemies.size(), enemy_lt.position - this_lt.position);

        if (!health.dots.contains(caster))
            health.dots[caster] = Stat(scene, 0.0f);
        health.dots[caster].add_effect(0, StatEffect{StatEffect::Type_Add, poison_dps, poison_max_stacks, poison_duration});
    }

    EmitterCPU emitter_cpu = load_asset<EmitterCPU>("emitters/assassin/basic_hit.sbemt");
    emitter_cpu.rotation = math::quat_between(v3(1.0f, 0.0f, 0.0f), math::normalize(hit_vec));
    quick_emitter(scene, "Assassin hit", v3(target) + v3(0.5f), emitter_cpu, 0.2f);

    auto poser = scene->registry.try_get<PoseController>(caster);
    if (poser) {
        if (phase == 0) {
            poser->set_state(AnimationState_AttackOut, post_trigger_time.value());
        } else {
            poser->set_state(AnimationState_Attack2Out, post_trigger_time.value());
        }
    }
    if (phase == 0) {
        phase = 1;
    } else {
        phase = 0;
    }
}

void AssassinAttack::end() {
    if (phase == 0) {
        post_trigger_timer->stop();
        request_cast();
    } else {
        auto poser = scene->registry.try_get<PoseController>(caster);
        if (poser) {
            poser->set_state(AnimationState_Idle, post_trigger_time.value());
        }
    }
    
}

void AssassinAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);

    if (taunted(*this, caster_comp))
        return;
    
    plus_targeting(1, *this, entry_gather_function);
}

void build_assassin(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    Caster& caster = scene->registry.get<Caster>(entity);
}

void draw_assassin_dragging_preview(Scene* scene, entt::entity entity) {
    v3 logic_pos = scene->registry.get<Dragging>(entity).potential_logic_position;
    
    vector<FormattedVertex> vertices;
    v3 pos = logic_pos + v3(0.5f, 0.5f, 0.02f);
    add_formatted_square(vertices, pos + v3(1.0f, 0.0f, 0.0f), v3(0.5f, 0.f, 0.f), v3(0.f, 0.5f, 0.f), palette::green_yellow, 0.05f);
    add_formatted_square(vertices, pos + v3(0.0f, 1.0f, 0.0f), v3(0.5f, 0.f, 0.f), v3(0.f, 0.5f, 0.f), palette::green_yellow, 0.05f);
    add_formatted_square(vertices, pos + v3(-1.0f, 0.0f, 0.0f), v3(0.5f, 0.f, 0.f), v3(0.f, 0.5f, 0.f), palette::green_yellow, 0.05f);
    add_formatted_square(vertices, pos + v3(0.0f, -1.0f, 0.0f), v3(0.5f, 0.f, 0.f), v3(0.f, 0.5f, 0.f), palette::green_yellow, 0.05f);
    if (vertices.empty())
        return;
    scene->render_scene.quick_mesh(generate_formatted_line(&scene->camera, vertices), true, false);
}


}
