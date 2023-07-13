#include <miniaudio.h>

#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>
#include <entt/core/hashed_string.hpp>

#include "extension/fmt.hpp"
#include "general/math/matrix_math.hpp"
#include "renderer/draw_functions.hpp"
#include "editor/console.hpp"
#include "game/game.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/area_trigger.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/consumer.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/tags.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/projectile.hpp"
#include "game/entities/targeting.hpp"

namespace spellbook {

using namespace entt::literals;

constexpr float attack_projectile_speed = 6.0f;
constexpr float attack_damage = 3.0f;
constexpr float attack_vuln_amount = 1.0f;

struct RangerAttack : Attack {
    using Attack::Attack;
    void targeting() override;
    void start() override;
    void trigger() override;
    float time_to_hit(v3i pos) override;

    string get_name() const override { return "Ranger Attack"; }
};

void RangerAttack::start() {
    lizard_turn_to_target();
}

void RangerAttack::trigger() {
    auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
    auto model = scene->registry.try_get<Model>(caster);
    auto ranger_model_transform = scene->registry.try_get<ModelTransform>(caster);
    auto& skeleton = model->model_cpu->skeleton;

    scene->audio.play_sound("audio/ranger/bow_trigger.wav", {.position = logic_tfm.position});
    
    v3 arrow_pos = logic_tfm.position;
    Bone* arrow_bone = skeleton->find_bone("Arrow");
    if (arrow_bone) {
        m44 t =  ranger_model_transform->get_transform() * model->model_cpu->root_node->cached_transform * arrow_bone->transform();
        arrow_pos = math::apply_transform(t, v3(0.0f, 0.0f, 0.0f));
        arrow_pos -= v3(0.5f);
    }

    Caster& caster_comp = scene->registry.get<Caster>(caster);
    
    Projectile projectile = {
        .target = target,
        .speed = StatInstance{&*caster_comp.projectile_speed, attack_projectile_speed},
        .alignment = v3(1.0f, 0.0f, 0.0f),
        .callback = [this](entt::entity proj_entity) {
            auto projectile = scene->registry.try_get<Projectile>(proj_entity);
            if (projectile) {
                auto logic_tfm = scene->registry.try_get<LogicTransform>(caster);
                
                scene->audio.play_sound("audio/ranger/arrow_impact.flac", {.position = v3(projectile->target)});
                
                EmitterCPU hit_emitter = load_asset<EmitterCPU>("emitters/ranger/basic_hit.sbemt");
                hit_emitter.set_velocity_direction(math::normalize(v3(projectile->target) - logic_tfm->position));
                quick_emitter(scene, "Ranger Basic Hit", v3(projectile->target) + v3(0.5f), hit_emitter, 0.1f);

                auto hit_enemies = entry_gather_function(*this, projectile->target, 0.0f);

                if (hit_enemies.size() == 0)
                    console({.str=fmt_("No enemies hit!")});
                
                // Apply damage
                for (entt::entity enemy : hit_enemies) {
                    auto health = scene->registry.try_get<Health>(enemy);
                    if (!health)
                        continue;
                    auto enemy_lt = scene->registry.try_get<LogicTransform>(enemy);
                    v3 damage_dir = v3(0.0f);
                    if (logic_tfm && enemy_lt)
                        damage_dir = enemy_lt->position - logic_tfm->position;
                    damage(scene, caster, enemy, attack_damage, damage_dir);
                }

                v3 pos = v3(projectile->target) + v3(0.5f, 0.5f, 0.05f);
                Scene* scene_cap = scene;
                add_tween_timer(scene, "ranger_hit", [pos, scene_cap](Timer* timer) {
                    vector<FormattedVertex> vertices;
                    float remaining_perc = timer->remaining_time / timer->total_time;
                    float width = math::clamp(remaining_perc * 2.0f, {0.0f, 1.0f}) * 0.05f;
                    add_formatted_square(vertices, pos, v3(0.40f, 0.f, 0.f), v3(0.f, 0.40f, 0.f), palette::steel_blue, width);
                    if (vertices.empty())
                        return;
                    scene_cap->render_scene.quick_mesh(generate_formatted_line(&scene_cap->camera, vertices), true, false);         
                }, true)->start(0.2f);
                
                // Vulnerability
                // Remove existing vulns
                for (auto [entity, enemy, health] : scene->registry.view<Enemy, Health>().each()) {
                    health.damage_taken_multiplier->remove_effect(uint32("ranger_mark"_hs));
                    auto& emitters = scene->registry.get<EmitterComponent>(entity);
                    emitters.remove_emitter(uint32("ranger_mark"_hs));
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
                    health.damage_taken_multiplier->add_effect(uint64("ranger_mark"_hs), StatEffect{
                        .type = StatEffect::Type_Multiply,
                        .value = attack_vuln_amount
                    });
                    auto& emitters = scene->registry.get<EmitterComponent>(select_enemy);
                    emitters.add_emitter(uint64("ranger_mark"_hs), load_asset<EmitterCPU>("emitters/ranger/basic_mark.sbemt"));
                }
            }
        }
    };
    quick_projectile(scene, projectile, arrow_pos, "emitters/ranger/basic_proj.sbemt", "models/hanther/hanther_arrow.sbmod", 0.5f);
}


void RangerAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    
    if (taunted(*this, caster_comp))
        return;

    if (square_targeting(3, *this, entry_gather_function, entry_eval_function))
        return;
}

float RangerAttack::time_to_hit(v3i pos) {
    auto& l_transform = scene->registry.get<LogicTransform>(caster);
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    float travel_time = math::distance(l_transform.position, v3(pos)) / stat_instance_value(&*caster_comp.projectile_speed, attack_projectile_speed);
    return pre_trigger_time.value() + travel_time;
}


struct RangerSpell : Spell {
    using Spell::Spell;
    void start() override;
    void trigger() override;
    void targeting() override;

    string get_name() const override { return "Ranger Spell"; }
};

void RangerSpell::start() {
    lizard_turn_to_target();
}

void RangerSpell::targeting() {
    // Ignore taunt
    trap_targeting(*this);
}

int ranger_attack_entry_eval(Scene* scene, const uset<entt::entity>& units) {
    int counter = 0;
    for (entt::entity entity : units) {
        entt::entity attachment_entity = scene->registry.get<Enemy>(entity).attachment;

        if (scene->registry.any_of<Egg>(attachment_entity))
            counter += 10;
        else
            counter += 1;
        
        Health& health = scene->registry.get<Health>(entity);
        bool marked = health.damage_taken_multiplier->effects.contains(uint64("ranger_mark"_hs));
        if (marked)
            counter += 3;
    }
    return counter;
}



void RangerSpell::trigger() {
    auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
    auto model = scene->registry.try_get<Model>(caster);
    auto ranger_model_transform = scene->registry.try_get<ModelTransform>(caster);
    auto& skeleton = model->model_cpu->skeleton;
    
    v3 trap_pos = logic_tfm.position;
    for (auto& bone : skeleton->bones) {
        if (bone->name == "Trap") {
            m44 t =  ranger_model_transform->get_transform() * model->model_cpu->root_node->cached_transform * bone->transform();
            trap_pos = math::apply_transform(t, v3(0.0f, 0.0f, 0.0f));
            trap_pos -= v3(0.5f);
        }
    }
    
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    
    Scene* scene_ptr = scene;
    entt::entity caster_entity = caster;
    Projectile projectile = {
        .target = target,
        .speed = StatInstance{&*caster_comp.projectile_speed, attack_projectile_speed},
        .alignment = v3(0.0f, 0.0f, 1.0f),
        .callback = [this, scene_ptr, caster_entity](entt::entity proj_entity) {
            auto projectile = scene_ptr->registry.try_get<Projectile>(proj_entity);
            if (!projectile)
                return;
            entt::registry& registry = scene_ptr->registry;
            entt::entity trap_entity = registry.create();

            static int i = 0;
            scene_ptr->registry.emplace<Name>(trap_entity, fmt_("trap_{}", i++));

            auto& model_comp = registry.emplace<Model>(trap_entity);
            model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>("models/hanther/hanther_trap.sbmod"));
            model_comp.model_gpu = instance_model(scene_ptr->render_scene, *model_comp.model_cpu);
            registry.emplace<PoseController>(trap_entity, *model_comp.model_cpu->skeleton);
            
            registry.emplace<LogicTransform>(trap_entity, v3(projectile->target));
            registry.emplace<ModelTransform>(trap_entity, v3(projectile->target), quat(), v3(0.5f));
            registry.emplace<TransformLink>(trap_entity, v3(0.75f, 0.25f, 0.0f));
    
            registry.emplace<EmitterComponent>(trap_entity, scene_ptr);
            
            registry.emplace<FloorOccupier>(trap_entity);
            AreaTrigger& trigger = registry.emplace<AreaTrigger>(trap_entity, scene_ptr);
            trigger.entity = trap_entity;
            trigger.caster_entity = caster_entity;
            trigger.entry_gather = area_trigger_gather_enemies();
            trigger.targeting = area_trigger_simple_targeting;
        
            trigger.trigger = [scene_ptr, trap_entity, caster_entity](AreaTrigger& area_trigger) {
                PoseController& poser = scene_ptr->registry.get<PoseController>(trap_entity);
                poser.set_state(AnimationState_AttackInto, 0.1f);
                
                add_timer(area_trigger.scene, "Ranger Trap Arm Timer", [trap_entity, caster_entity](Timer* timer) {
                    AreaTrigger& area_trigger = timer->scene->registry.get<AreaTrigger>(trap_entity); 
                    LogicTransform& logic_tfm = timer->scene->registry.get<LogicTransform>(area_trigger.entity);

                    uset<entt::entity> enemies = area_trigger.entry_gather(area_trigger, math::round_cast(logic_tfm.position));
                    for (entt::entity enemy : enemies) {
                        damage(timer->scene, caster_entity, enemy, 2.0f, v3(0, 0, 1));

                        Tags& tags = timer->scene->registry.get<Tags>(enemy);
                        tags.apply_tag("no_move"_hs, "Ranger Trap Root"_hs, 3.0f);
                        tags.apply_tag("no_cast"_hs, "Ranger Trap Silence"_hs, 3.0f);
                    }
                }, true)->start(0.5f);
                add_timer(area_trigger.scene, "Ranger Trap Dissipate Timer", [trap_entity](Timer* timer) {
                    timer->scene->registry.emplace<Killed>(trap_entity);
                }, true)->start(0.8f);
            };

        }
    };
    quick_projectile(scene, projectile, trap_pos, "emitters/ranger/basic_proj.sbemt", "models/hanther/hanther_trap.sbmod", 0.3f);
}

void build_ranger(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    Caster& caster = scene->registry.get<Caster>(entity);

    caster.attack = std::make_unique<RangerAttack>(scene, entity, 0.8f, 1.3f, 1.0f, 4.0f);
    caster.attack->entry_gather_function = gather_enemies();
    caster.attack->entry_eval_function = ranger_attack_entry_eval;

    caster.spell = std::make_unique<RangerSpell>(scene, entity, 0.5f, 0.5f);
    caster.spell->entry_gather_function = gather_enemies();
    caster.spell->entry_eval_function = basic_lizard_entry_eval;
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
