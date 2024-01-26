#include "enemy.hpp"

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/core/hashed_string.hpp>
#include <imgui.h>
#include <tracy/Tracy.hpp>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "editor/pose_widget.hpp"
#include "general/astar.hpp"
#include "general/color.hpp"
#include "general/math/matrix_math.hpp"
#include "renderer/draw_functions.hpp"
#include "game/game.hpp"
#include "game/pose_controller.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/consumer.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/enemy_ik.hpp"
#include "game/entities/tags.hpp"
#include "game/entities/targeting.hpp"


namespace spellbook {

using namespace entt::literals;

struct EnemyLaserAttack : Attack {
    std::shared_ptr<Timer> beam_animate;

    using Attack::Attack;
    void targeting() override;
    void trigger() override;
};

void EnemyLaserAttack::trigger() {
    auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
    scene->audio.play_sound("audio/enemy/laser.flac"_resource, {.position = logic_tfm.position});
    
    uset<entt::entity> lizards = entry_gather_function(*this, target, 0.0f);
    for (entt::entity lizard : lizards) {
        LogicTransform& liz_tfm = scene->registry.get<LogicTransform>(lizard);
        Health& health = scene->registry.get<Health>(lizard);
        health.damage(caster, 1.0f, liz_tfm.position - logic_tfm.position);

        constexpr float duration = 0.15f;
        entt::entity caster_cap = caster;
        beam_animate = add_tween_timer(scene, "Enemy laser tick", [lizard, caster_cap](Timer* timer) {
            if (timer->ticking && timer->scene->registry.valid(lizard) && timer->scene->registry.valid(caster_cap)) {
                auto* logic_tfm = timer->scene->registry.try_get<LogicTransform>(caster_cap);
                auto* liz_tfm = timer->scene->registry.try_get<LogicTransform>(lizard);
                if (logic_tfm && liz_tfm) {
                    vector<FormattedVertex> vertices;
                    float width = (timer->remaining_time / timer->total_time) * 0.05f + 0.03f;
                    vertices.emplace_back(logic_tfm->position + v3(0.5f, 0.5f, 0.19f), palette::light_pink, width + 0.05f);
                    vertices.emplace_back(liz_tfm->position + v3(0.5f), palette::red, width);
                    timer->scene->render_scene.quick_mesh(generate_formatted_line(&timer->scene->camera, vertices), true, false);
                }
            }
        }, false);
        beam_animate->start(duration);
    }
}


void EnemyLaserAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);

    if (taunted(*this, caster_comp))
        return;

    square_targeting(1, *this, entry_gather_function, entry_eval_function);
}

struct EnemyResistorAttack : Attack {
    std::shared_ptr<Timer> aura_animate;

    using Attack::Attack;
    void trigger() override;
    void targeting() override;
    bool can_cast() const override;
};

void EnemyResistorAttack::trigger() {
    LogicTransform& logic_tfm = scene->registry.get<LogicTransform>(caster);
    scene->audio.play_sound("audio/enemy/resistor.flac"_resource, {.position = logic_tfm.position, .volume = 0.5f});
    
    constexpr float buff_duration = 2.0f;
    uset<entt::entity> enemies = entry_gather_function(*this, target, 0.0f);
    for (entt::entity enemy : enemies) {
        Health& health = scene->registry.get<Health>(enemy);
        health.damage_taken_multiplier->add_effect((uint64) this, StatEffect(StatEffect::Type_Multiply, -0.3f, INT_MAX, buff_duration));
    }
    
    constexpr float duration = 0.25f;
    entt::entity caster_cap = caster;
    aura_animate = add_tween_timer(scene, "Enemy laser tick", [caster_cap](Timer* timer) {
        if (timer->ticking && timer->scene->registry.valid(caster_cap) && timer->scene->registry.valid(caster_cap)) {
            if (!timer->scene->registry.valid(caster_cap))
                return;
            LogicTransform& logic_tfm = timer->scene->registry.get<LogicTransform>(caster_cap);
            float t = 1.0f - timer->remaining_time / timer->total_time;
            vector<FormattedVertex> vertices;
            Color color = mix(palette::red, palette::pink, t);
            float radius = math::mix(0.25f, 1.5f, math::ease(t, math::EaseMode_CubicOut));
            float line_width = 0.05f;
            add_formatted_square(vertices, logic_tfm.position + v3(0.5f, 0.5f, 0.0f), v3(radius, 0.f, 0.f), v3(0.f, radius, 0.f), color, line_width);
            if (vertices.empty())
                return;
            timer->scene->render_scene.quick_mesh(generate_formatted_line(&timer->scene->camera, vertices), true, false);
        }
    }, false);
    aura_animate->start(duration);
}

void EnemyResistorAttack::targeting() {
    has_target = false;
}

bool EnemyResistorAttack::can_cast() const {
    if (cooldown_timer && cooldown_timer->ticking)
        return false;
    Tags& tags = scene->registry.get<Tags>(caster);
    if (tags.has_tag("no_cast"_hs))
        return false;
    if (casting())
        return false;
    return true;
}

struct EnemyMortarAttack : Attack {
    std::shared_ptr<Timer> beam1_animate;
    std::shared_ptr<Timer> beam2_animate;
    std::shared_ptr<Timer> indicator_animate;
    std::shared_ptr<Timer> trigger_timer;

    using Attack::Attack;
    void targeting() override;
    void trigger() override;
};

void EnemyMortarAttack::trigger() {
    constexpr float beam_duration = 0.2f;
    entt::entity caster_cap = caster;
    beam1_animate = add_tween_timer(scene, "Enemy mortar beam 1", [caster_cap](Timer* timer) {
        if (timer->ticking && timer->scene->registry.valid(caster_cap)) {
            LogicTransform& caster_logic_tfm = timer->scene->registry.get<LogicTransform>(caster_cap);
            vector<FormattedVertex> vertices;
            float width = (1.0f - timer->remaining_time / timer->total_time) * 0.05f + 0.05f;
            vertices.emplace_back(caster_logic_tfm.position + v3(0.5f, 0.5f, 0.0f), palette::light_pink, width);
            vertices.emplace_back(caster_logic_tfm.position + v3(0.5f, 0.5f, 0.0f) + v3::Z * 1.5f, palette::red, 0.0f);
            timer->scene->render_scene.quick_mesh(generate_formatted_line(&timer->scene->camera, vertices), true, false);
        }
    }, false);
    beam1_animate->start(beam_duration);

    v3i position_cap = target;
    constexpr float indicator_duration = 1.5f;
    indicator_animate = add_tween_timer(scene, "Enemy mortar indicator", [position_cap](Timer* timer) {
        if (timer->ticking) {
            float t = 1.0f - timer->remaining_time / timer->total_time;
            
            vector<FormattedVertex> vertices;
            Color color = mix(palette::pink, palette::red, t);
            float line_width = math::mix(0.02f, 0.06f, t);
            float radius = math::mix(0.4f, 0.5f, t);
            add_formatted_square(vertices, v3(position_cap) + v3(0.5f, 0.5f, 0.02f), v3(radius, 0.0f, 0.f), v3(0.f, radius, 0.f), color, line_width);
            if (vertices.empty())
                return;
            timer->scene->render_scene.quick_mesh(generate_formatted_line(&timer->scene->camera, vertices), true, false);
        }
    }, false);
    indicator_animate->start(indicator_duration);

    trigger_timer = add_timer(scene, "Enemy mortar trigger", [this, position_cap](Timer* timer) {
        if (!timer->scene->registry.valid(caster))
            return;
        uset<entt::entity> lizards = entry_gather_function(*this, position_cap, 0.0f);
        for (entt::entity lizard : lizards) {
            LogicTransform& logic_tfm = timer->scene->registry.get<LogicTransform>(caster);
            LogicTransform& liz_tfm = timer->scene->registry.get<LogicTransform>(lizard);
            Health& health = timer->scene->registry.get<Health>(lizard);
        
            health.damage(caster, 5.0f, liz_tfm.position - logic_tfm.position);
        }
        
        beam2_animate = add_tween_timer(timer->scene, "Enemy mortar beam 2", [position_cap](Timer* timer) {
            if (timer->ticking) {
                vector<FormattedVertex> vertices;
                float width = (timer->remaining_time / timer->total_time) * 0.1f + 0.1f;
                float height = (timer->remaining_time / timer->total_time) * 1.0f + 0.5f;
                vertices.emplace_back(v3(position_cap) + v3(0.5f, 0.5f, 0.0f), palette::light_pink, width);
                vertices.emplace_back(v3(position_cap) + v3(0.5f, 0.5f, 0.0f) + v3::Z * height, palette::red, 0.0f);
                timer->scene->render_scene.quick_mesh(generate_formatted_line(&timer->scene->camera, vertices), true, false);
            }
        }, false);
        beam2_animate->start(beam_duration);
        
        const EmitterCPU& hit_emitter = load_resource<EmitterCPU>("emitters/enemy/mortar_hit.sbjemt"_resource);
        quick_emitter(scene, "Mortar Hit", v3(position_cap) + v3(0.5f, 0.5f, 0.5f), hit_emitter, 0.1f);
    }, false);
    trigger_timer->start(indicator_duration);
}


void EnemyMortarAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);

    if (taunted(*this, caster_comp))
        return;

    square_targeting(5, *this, entry_gather_function, entry_eval_function);
}


entt::entity instance_prefab(Scene* scene, const EnemyPrefab& prefab, v3i location) {
    static SpiderControllerSettings settings;

    vector<PathInfo*> available_paths;
    for (PathInfo& path : scene->paths) {
        if (math::distance(path.path.get_start(), v3(location)) < 0.1f)
            available_paths.push_back(&path);
    }
    PathInfo* selected_path = !available_paths.empty() ? available_paths[math::random_int32(available_paths.size())] : nullptr;
    entt::entity spawner = selected_path ? selected_path->spawner : entt::null;
    entt::entity selected_consumer = selected_path ? selected_path->consumer : entt::null;
        
    entt::entity base_entity = setup_basic_unit(scene, prefab.base_model_path, v3(location), prefab.max_health, prefab.hurt_path);
    static int base_i = 0;
    scene->registry.emplace<Name>(base_entity, fmt_("{}_{}", prefab.base_model_path.stem(), base_i++));
    entt::entity attachment_entity = scene->registry.create();
    scene->registry.erase<TransformLink>(base_entity);
    scene->registry.emplace<Enemy>(base_entity, attachment_entity, spawner, selected_consumer);
    scene->registry.emplace<Traveler>(base_entity);
    scene->registry.emplace<SpiderController>(base_entity, settings);
    
    scene->registry.get<Traveler>(base_entity).max_speed = std::make_unique<Stat>(scene, base_entity, prefab.max_speed);
    scene->registry.get<ModelTransform>(base_entity).scale = v3(prefab.base_scale);

    if (!prefab.drops.entries.empty())
        scene->registry.emplace<DropChance>(base_entity, prefab.drops);
    
    auto& model_comp = scene->registry.emplace<Model>(attachment_entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_resource<ModelCPU>(prefab.attachment_model_path));
    model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);

    static int attachment_i = 0;
    scene->registry.emplace<Name>(attachment_entity, fmt_("{}_{}", prefab.attachment_model_path.stem(), attachment_i++));
    scene->registry.emplace<AddToInspect>(attachment_entity);
    scene->registry.emplace<Tags>(attachment_entity, *scene);
    scene->registry.emplace<Attachment>(attachment_entity, base_entity);
    scene->registry.emplace<LogicTransform>(attachment_entity, v3(location));
    scene->registry.emplace<ModelTransform>(attachment_entity);
    scene->registry.get<ModelTransform>(attachment_entity).scale = v3(prefab.attachment_scale);
    if (model_comp.model_cpu->skeleton) {
        scene->registry.emplace<PoseController>(attachment_entity, *model_comp.model_cpu->skeleton);
    }
    
    switch (prefab.type) {
        case (EnemyType_Laser): {
            Caster& caster = scene->registry.emplace<Caster>(attachment_entity, scene, attachment_entity);
            caster.attack = std::make_unique<EnemyLaserAttack>(scene, attachment_entity, 0.5f, 1.0f, 1.5f, 1.0f);
            caster.attack->entry_gather_function = gather_lizard();
            caster.attack->entry_eval_function = simple_entry_eval;
        } break;
        case (EnemyType_Resistor): {
            Caster& caster = scene->registry.emplace<Caster>(attachment_entity, scene, attachment_entity);
            caster.attack = std::make_unique<EnemyResistorAttack>(scene, attachment_entity, 0.1f, 0.1f, 1.5f, 1.0f);
            caster.attack->entry_gather_function = gather_enemies_aoe(1);
            caster.attack->entry_eval_function = simple_entry_eval;
        } break;
        case (EnemyType_Mortar): {
            Caster& caster = scene->registry.emplace<Caster>(attachment_entity, scene, attachment_entity);
            caster.attack = std::make_unique<EnemyMortarAttack>(scene, attachment_entity, 0.75f, 0.75f, 4.0f, 5.0f);
            caster.attack->entry_gather_function = gather_lizard();
            caster.attack->entry_eval_function = simple_entry_eval;
        } break;
        default: {
        } break;
    }
    
    return base_entity;  
}

bool inspect(EnemyPrefab* enemy_prefab) {
    bool changed = false;
    ImGui::PathSelect<EnemyPrefab>("File", &enemy_prefab->file_path);
    changed |= inspect_dependencies(enemy_prefab->dependencies, enemy_prefab->file_path);
    changed |= ImGui::EnumCombo("Type", &enemy_prefab->type);
    changed |= ImGui::PathSelect<ModelCPU>("Base Model", &enemy_prefab->base_model_path);
    changed |= ImGui::PathSelect<ModelCPU>("Attachment Model", &enemy_prefab->attachment_model_path);
    changed |= ImGui::PathSelect<EmitterCPU>("Hurt", &enemy_prefab->hurt_path);
    changed |= ImGui::DragFloat("Max Health", &enemy_prefab->max_health, 0.01f, 0.0f);
    changed |= ImGui::DragFloat("Max Speed", &enemy_prefab->max_speed, 0.01f, 0.0f);
    changed |= ImGui::DragFloat("Base Scale", &enemy_prefab->base_scale, 0.01f, 0.0f);
    changed |= ImGui::DragFloat("Attachment Scale", &enemy_prefab->attachment_scale, 0.01f, 0.0f);
    changed |= inspect(&enemy_prefab->drops);
    
    return changed;
}

void enemy_ik_controller_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, logic_tfm, model_tfm, model, ik] : scene->registry.view<LogicTransform, ModelTransform, Model, SpiderController>().each()) {
        if (!model.model_cpu->skeleton)
            continue;

        assert_else(!math::is_nan(ik.extra_z))
            ik.extra_z = 0.0f;
        assert_else(!math::is_nan(ik.quad_norm))
            ik.quad_norm = v3::Z;

        Grounded* grounded = scene->registry.try_get<Grounded>(entity);
        v3 step_up = grounded ? v3::Z * grounded->step_up : v3{};
        
        model_tfm.translation = logic_tfm.position + v3(0.5f, 0.5f, 0.0f * ik.extra_z) + step_up;
        model_tfm.rotation = math::quat_between(v3::Z, ik.quad_norm);
        model_tfm.set_dirty();

        m44 tfm = model_tfm.get_transform();
        m44 tfm_inv = math::inverse(tfm);

        bool set0_moving = ik.is_set_moving(0);
        bool set1_moving = ik.is_set_moving(1);

        ik.initialize_if_needed(tfm);
        ik.update_velocity(scene, logic_tfm);
        ik.update_desire(tfm);
        ik.resolve_desire();
        ik.update_target(scene, logic_tfm);
        assert_else(ik.check_all_targets_set());
        ik.update_transform_linkers();
        ik.update_constraints(scene, model, tfm_inv);

        if ((set0_moving && !ik.is_set_moving(0)) || (set1_moving && !ik.is_set_moving(1))) {
            scene->audio.play_sound("audio/enemy/step.wav"_resource, {.position = logic_tfm.position});
        }
    }
}

void attachment_transform_system(Scene* scene) {
    ZoneScoped;
    for (auto [base_entity, logic_tfm, enemy] : scene->registry.view<LogicTransform, Enemy>().each()) {
        if (!scene->registry.valid(enemy.attachment))
            continue;
        
        Model* model = scene->registry.try_get<Model>(base_entity);
        ModelTransform* model_tfm = scene->registry.try_get<ModelTransform>(base_entity);
        SpiderController* ik = scene->registry.try_get<SpiderController>(base_entity);

        Caster* caster = scene->registry.try_get<Caster>(enemy.attachment);
        LogicTransform& attachment_logic_tfm = scene->registry.get<LogicTransform>(enemy.attachment);
        ModelTransform& attachment_model_tfm = scene->registry.get<ModelTransform>(enemy.attachment);
        
        attachment_logic_tfm.position = logic_tfm.position;
        attachment_logic_tfm.normal = ik ? ik->quad_norm : v3::Z;
        if (caster && caster->attack->has_target) {
            v3 vec = math::normalize(v3(caster->attack->target) - attachment_logic_tfm.position);
            attachment_logic_tfm.yaw = math::atan2(vec.y, vec.x);
        } else if (ik && math::length(ik->velocity) > 0.01f) {
            v3 vec = math::normalize(ik->velocity);
            attachment_logic_tfm.yaw = math::atan2(vec.y, vec.x);
        }

        if (model && model_tfm) {
            m44 tfm = model_tfm->get_transform();
            Bone* anchor_bone = model->model_cpu->skeleton->find_bone("anchor");
            if (anchor_bone) {
                m44 t =  tfm * model->model_cpu->root_node->cached_transform * anchor_bone->transform();
                attachment_model_tfm.set_translation(math::apply_transform(t, v3(0.0f, 0.0f, 0.0f)));
            }
            quat logic_quat = math::to_quat(math::normal_yaw(attachment_logic_tfm.normal, attachment_logic_tfm.yaw - math::PI / 2.0f));
            assert_else(!math::is_nan(logic_quat))
                logic_quat = {};
            attachment_model_tfm.set_rotation(logic_quat);
        }
    }
}

void enemy_aggro_system(Scene* scene) {
    ZoneScoped;
    if (scene->edit_mode)
        return;
    // manage movement requests from attachment
    for (auto [entity, attachment, caster] : scene->registry.view<Attachment, Caster>().each()) {
        Attack* attack = (Attack*) &*caster.attack;
        if (!caster.attack->has_target || caster.attack->in_range() || attack->cooldown_timer->ticking)
            continue;

        if (!scene->registry.valid(attachment.base))
            continue;
        
        Traveler* traveler = scene->registry.try_get<Traveler>(attachment.base);
        if (!traveler)
            continue;
        traveler->set_target({5, caster.attack->target});
    }

    // bring egg back if we have it, otherwise seek egg
    for (auto [entity, enemy, traveler] : scene->registry.view<Enemy, Traveler>().each()) {
        bool has_egg = scene->registry.any_of<Egg>(enemy.attachment);
        if (has_egg) {
            LogicTransform& spawner_tfm = scene->registry.get<LogicTransform>(enemy.from_spawner);
            traveler.set_target({2, math::round_cast(spawner_tfm.position)});
            continue;
        }
        if (!scene->registry.valid(enemy.target_consumer))
            continue;
        Shrine& shrine = scene->registry.get<Shrine>(enemy.target_consumer);
        LogicTransform& egg_tfm = scene->registry.get<LogicTransform>(shrine.egg_entity);
        traveler.set_target({10, math::round_cast(egg_tfm.position)});
    }

    // Pick up egg if possible
    for (auto [entity, enemy, traveler, logic_tfm] : scene->registry.view<Enemy, Traveler, LogicTransform>().each()) {
        entt::entity egg_entity = scene->registry.get<Shrine>(enemy.target_consumer).egg_entity;
        Attachment& egg_attachment = scene->registry.get<Attachment>(egg_entity);
        // another enemy is carrying the egg
        if (scene->registry.valid(egg_attachment.base) && egg_attachment.base != enemy.target_consumer)
            continue;
        LogicTransform& egg_tfm = scene->registry.get<LogicTransform>(egg_entity);
        if (math::distance(egg_tfm.position, logic_tfm.position) < 0.1f) {
            disconnect_attachment(scene, entity);
            disconnect_attachment(scene, enemy.target_consumer);
            connect_attachment(scene, entity, egg_entity);
        }
    }
}

void traveler_reset_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, traveler] : scene->registry.view<Traveler>().each()) {
        traveler.reset_target();
    }
}

void travel_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, transform, traveler] : scene->registry.view<LogicTransform, Traveler>().each()) {
        if (scene->registry.any_of<Tags>(entity) && scene->registry.get<Tags>(entity).has_tag("no_move"_hs))
            continue;
        if (!traveler.has_target())
            continue;
        
        // Update pathing from target
        if (!traveler.path.valid() || math::round_cast(traveler.path.get_destination()) != traveler.target.pos) {
            traveler.path = scene->navigation->find_path(math::round_cast(transform.position), traveler.target.pos);
        }

        if (!traveler.path.valid())
            continue;

        Enemy* enemy = scene->registry.try_get<Enemy>(entity);
        Caster* caster = enemy ? scene->registry.try_get<Caster>(enemy->attachment) : nullptr;
        if (caster && caster->attack->casting())
            continue;
        
        // Use pathing to move
        v3   velocity        = v3(traveler.path.get_real_target(transform.position)) - transform.position;
        bool at_target       = math::length(velocity) < 0.05f;
        if (at_target) {
            velocity = v3(0);
            traveler.path.reached--;
        }
        float max_velocity = traveler.max_speed->value() * scene->delta_time;
        float min_velocity = 0.0f;
        if (!at_target)
            transform.position += math::normalize(velocity) * math::clamp(math::length(velocity), min_velocity, max_velocity);
    }
}

void enemy_decollision_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity1, enemy1, logic_tfm1, traveler1] : scene->registry.view<Enemy, LogicTransform, Traveler>().each()) {
        for (auto [entity2, enemy2, logic_tfm2, traveler2] : scene->registry.view<Enemy, LogicTransform, Traveler>().each()) {
            if (entity1 == entity2)
                continue;
            entt::entity target_egg = scene->registry.get<Shrine>(enemy2.target_consumer).egg_entity;
            if (enemy2.attachment == target_egg)
                continue;
            float dist = math::distance(logic_tfm1.position, logic_tfm2.position);
            float push_amount = scene->delta_time * math::map_range(dist, {0.0f, 0.4f}, {1.0f, 0.0f});
            if (push_amount < scene->delta_time * 0.1f)
                continue;
            v2 jitter = {math::random_float(0.1f), math::random_float(0.1f)};
            v3 push_dir = math::normalize(v3(logic_tfm2.position.xy - logic_tfm1.position.xy + jitter, 0.0f));
            logic_tfm2.position += push_amount * push_dir;
        }
    }
}

v3 predict_pos(Traveler& traveler, v3 pos, float time) {
    v3 expected_pos = pos;
    if (time == 0.0f)
        return expected_pos;
    if (traveler.path.waypoints.empty())
        return expected_pos;
    for (int32 i = traveler.path.reached; i > 0; i--) {
        // TODO: If we're not using pathing, just return pos
        v3 pos1 = i == traveler.path.reached ? pos : v3(traveler.path.waypoints[i]);
        v3 pos2 = v3(traveler.path.waypoints[i-1]);
        float time_delta = math::distance(v3(pos1), v3(pos2)) / traveler.max_speed->value();
        if (time_delta > time) {
            expected_pos = math::lerp(time / time_delta, range3{v3(pos1), v3(pos2)});
            break;
        }
        time -= time_delta;
        expected_pos = pos2;
    }
    return expected_pos;
}

void on_enemy_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    disconnect_attachment(&scene, entity);
}

void disconnect_attachment(Scene* scene, Enemy& enemy) {
    if (!scene->registry.valid(enemy.attachment))
        return;
    Attachment* attachment = scene->registry.try_get<Attachment>(enemy.attachment);
    if (attachment) {
        attachment->base = entt::null;
        if (attachment->requires_base)
            scene->registry.emplace<Killed>(enemy.attachment);
    }
    enemy.attachment = entt::null;
}
void disconnect_attachment(Scene* scene, Shrine& shrine) {
    if (!scene->registry.valid(shrine.egg_entity) || !shrine.egg_attached)
        return;
    
    Attachment* attachment = scene->registry.try_get<Attachment>(shrine.egg_entity);
    if (attachment) {
        attachment->base = entt::null;
        assert_else(!attachment->requires_base);
    }
    shrine.egg_attached = false;
}

void disconnect_attachment(Scene* scene, entt::entity base) {
    Enemy* enemy = scene->registry.try_get<Enemy>(base);
    Shrine* shrine = scene->registry.try_get<Shrine>(base);
    assert_else((enemy != nullptr) != (shrine != nullptr))
        return;

    if (enemy)
        disconnect_attachment(scene, *enemy);

    if (shrine)
        disconnect_attachment(scene, *shrine);
}

void connect_attachment(Scene* scene, Enemy& enemy, entt::entity base, entt::entity attach_entity) {
    Attachment& attachment = scene->registry.get<Attachment>(attach_entity);
    assert_else(!scene->registry.valid(enemy.attachment));
    assert_else(!scene->registry.valid(attachment.base));
    enemy.attachment = attach_entity;
    attachment.base = base;
}

void connect_attachment(Scene* scene, Shrine& shrine, entt::entity base, entt::entity attach_entity) {
    Attachment& attachment = scene->registry.get<Attachment>(attach_entity);
    assert_else(!scene->registry.valid(attachment.base));
    shrine.egg_attached = true;
    attachment.base = base;
}

void connect_attachment(Scene* scene, entt::entity base, entt::entity attachment) {
    Enemy* enemy = scene->registry.try_get<Enemy>(base);
    Shrine* shrine = scene->registry.try_get<Shrine>(base);
    assert_else((enemy != nullptr) != (shrine != nullptr))
        return;

    if (enemy)
        connect_attachment(scene, *enemy, base, attachment);

    if (shrine)
        connect_attachment(scene, *shrine, base, attachment);
}

void inspect(Traveler* traveler) {
    if (traveler->has_target())
        ImGui::DragInt3("Target", traveler->target.pos.data, 0.01f);
    ImGui::Text("Pathing size: %d", traveler->path.waypoints.size());
    if (ImGui::TreeNode("Speed")) {
        inspect(&*traveler->max_speed);
        ImGui::TreePop();
    }
}

}
