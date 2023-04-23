#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "extension/fmt.hpp"
#include "renderer/draw_functions.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/projectile.hpp"
#include "game/entities/targeting.hpp"
#include "general/matrix_math.hpp"

namespace spellbook {

const float base_projectile_speed = 6.0f;

struct RangerAttack : Ability {
    void targeting() override;
    void start() override;
    void trigger() override;
    float time_to_hit(v3i pos) override;
};

void RangerAttack::start() {
    v3i caster_pos = math::round_cast(scene->registry.get<LogicTransform>(caster).position);
    auto lizard = scene->registry.try_get<Lizard>(caster);
    if (lizard) {
        v3 dir_to = math::normalize(v3(target) - v3(caster_pos));
        float ang = math::angle_difference(lizard->default_direction.xy, dir_to.xy);
        scene->registry.get<LogicTransform>(caster).rotation.yaw = ang;
    }
}

void RangerAttack::trigger() {
    auto& ranger_logic_transform = scene->registry.get<LogicTransform>(caster);
    auto model = scene->registry.try_get<Model>(caster);
    auto ranger_model_transform = scene->registry.try_get<ModelTransform>(caster);
    auto& skeleton = model->model_cpu->skeleton;

    v3 arrow_pos = ranger_logic_transform.position;
    for (auto& bone : skeleton->bones) {
        if (bone->name == "Arrow") {
            m44 t =  ranger_model_transform->get_transform() * model->model_cpu->root_node->cached_transform * bone->transform();
            arrow_pos = math::apply_transform(t, v3(0.0f, 0.0f, 0.0f));
            arrow_pos -= v3(0.5f);
        }
    }

    Caster& caster_comp = scene->registry.get<Caster>(caster);
    
    Projectile projectile = {
        .target = target,
        .speed = StatInstance{&*caster_comp.projectile_speed, base_projectile_speed},
        .alignment = v3(1.0f, 0.0f, 0.0f),
        .callback = [this](entt::entity proj_entity) {
            auto projectile = scene->registry.try_get<Projectile>(proj_entity);
            if (projectile) {
                auto this_lt = scene->registry.try_get<LogicTransform>(caster);
                EmitterCPU hit_emitter = load_asset<EmitterCPU>("emitters/ranger/basic_hit.sbemt");
                hit_emitter.set_velocity_direction(math::normalize(v3(projectile->target) - this_lt->position));
                quick_emitter(scene, "Ranger Basic Hit", v3(projectile->target) + v3(0.5f), hit_emitter, 0.1f);

                u64 vuln_id = hash_string("Ranger Basic Mark");
                
                auto hit_enemies = entry_gather_function(*this, projectile->target, 0.0f);

                // Apply damage
                for (entt::entity enemy : hit_enemies) {
                    auto health = scene->registry.try_get<Health>(enemy);
                    if (!health)
                        continue;
                    auto enemy_lt = scene->registry.try_get<LogicTransform>(enemy);
                    v3 damage_dir = v3(0.0f);
                    if (this_lt && enemy_lt)
                        damage_dir = enemy_lt->position - this_lt->position;
                    damage(scene, caster, enemy, 3.0f, damage_dir);
                }

                v3 pos = v3(projectile->target) + v3(0.5f, 0.5f, 0.05f);
                add_tween_timer(scene, "ranger_hit", [pos, this](Timer* timer) {
                    vector<FormattedVertex> vertices;
                    float remaining_perc = timer->remaining_time / timer->total_time;
                    float width = math::clamp(remaining_perc * 2.0f, {0.0f, 1.0f}) * 0.05f;
                    add_formatted_square(vertices, pos, v3(0.40f, 0.f, 0.f), v3(0.f, 0.40f, 0.f), palette::steel_blue, width);
                    if (vertices.empty())
                        return;
                    scene->render_scene.quick_mesh(generate_formatted_line(&scene->camera, vertices), true, false);         
                }, false).start(0.2f);
                
                // Vulnerability
                // Remove existing vulns
                for (auto [entity, health] : scene->registry.view<Enemy, Health>().each()) {
                    health.damage_taken_multiplier.remove_effect(vuln_id);
                    auto& emitters = scene->registry.get<EmitterComponent>(entity);
                    emitters.remove_emitter(vuln_id);
                }
                // Select vuln
                entt::entity select_enemy = entt::null;
                float select_health = -FLT_MAX;
                for (entt::entity enemy : hit_enemies) {
                    Health& health = scene->registry.get<Health>(enemy);
                    if (health.value > select_health) {
                        select_health = health.value;
                        select_enemy = enemy;
                    }
                }
                // Apply vuln
                if (scene->registry.valid(select_enemy)) {
                    Health& health = scene->registry.get<Health>(select_enemy);
                    health.damage_taken_multiplier.add_effect(vuln_id, StatEffect{
                        .type = StatEffect::Type_Multiply,
                        .value = 1.0f
                    });
                    auto& emitters = scene->registry.get<EmitterComponent>(select_enemy);
                    emitters.add_emitter(vuln_id, load_asset<EmitterCPU>("emitters/ranger/basic_mark.sbemt"));
                }
            }
        }
    };
    quick_projectile(scene, projectile, arrow_pos, "emitters/ranger/basic_proj.sbemt", "models/hanther/hanther_arrow.sbmod");
}


void RangerAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    
    if (taunted(*this, caster_comp))
        return;

    if (square_targeting(3, *this, entry_gather_function))
        return;
}

float RangerAttack::time_to_hit(v3i pos) {
    auto& l_transform = scene->registry.get<LogicTransform>(caster);
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    float travel_time = math::distance(l_transform.position, v3(pos)) / stat_instance_value(&*caster_comp.projectile_speed, base_projectile_speed);
    return pre_trigger_time.value() + travel_time;
}

struct RangerAbility : Ability {
};



void build_ranger(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    Caster& caster = scene->registry.get<Caster>(entity);

    caster.attack = std::make_unique<RangerAttack>();
    caster.attack->setup(scene, entity, 0.8f, 1.3f, Ability::Type_Attack);
    caster.attack->entry_gather_function = lizard_entry_gather();

    caster.ability = std::make_unique<RangerAbility>();
    caster.ability->setup(scene, entity, 1.0f, 1.0f, Ability::Type_Ability);
    caster.ability->entry_gather_function = lizard_entry_gather();
}

void draw_ranger_dragging_preview(Scene* scene, entt::entity entity) {
    v3 logic_pos = scene->registry.get<Dragging>(entity).potential_logic_position;
    
    vector<FormattedVertex> vertices;
    v3 pos = logic_pos + v3(0.5f, 0.5f, 0.05f);
    add_formatted_square(vertices, pos, v3(3.5f, 0.f, 0.f), v3(0.f, 3.5f, 0.f), palette::steel_blue, 0.05f);
    if (vertices.empty())
        return;
    scene->render_scene.quick_mesh(generate_formatted_line(&scene->camera, vertices), true, false);
}

}
