#include "enemy.hpp"

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <imgui.h>

#include "caster.hpp"
#include "enemy_ik.hpp"
#include "impair.hpp"
#include "targeting.hpp"
#include "editor/pose_widget.hpp"
#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "game/pose_controller.hpp"
#include "general/astar.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/consumer.hpp"
#include "general/matrix_math.hpp"
#include "renderer/draw_functions.hpp"


namespace spellbook {

struct EnemyLaserAttack : Attack {
    std::shared_ptr<Timer> beam_animate;

    using Attack::Attack;
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
        
        damage(scene, caster, lizard, 1.0f, liz_lt.position - this_lt.position);

        constexpr float duration = 0.15f;
        entt::entity caster_v = caster;
        beam_animate = add_tween_timer(scene, "Enemy laser tick", [this, lizard, caster_v](Timer* timer) {
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
        }, false);
        beam_animate->start(duration);
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
    static SpiderControllerSettings settings;

    vector<PathInfo*> available_paths;
    for (PathInfo& path : scene->paths) {
        if (math::distance(path.path.back(), v3(location)) < 0.1f)
            available_paths.push_back(&path);
    }
    entt::entity selected_consumer = !available_paths.empty() ? available_paths[math::random_s32(available_paths.size())]->consumer : entt::null;
        
    entt::entity base_entity = setup_basic_unit(scene, prefab.base_model_path, v3(location), prefab.max_health, prefab.hurt_path);
    entt::entity attachment_entity = scene->registry.create();
    scene->registry.erase<TransformLink>(base_entity);
    scene->registry.emplace<Enemy>(base_entity, attachment_entity, selected_consumer);
    scene->registry.emplace<Traveler>(base_entity);
    scene->registry.emplace<SpiderController>(base_entity, settings);
    
    scene->registry.get<Traveler>(base_entity).max_speed = Stat(scene, prefab.max_speed);
    scene->registry.get<ModelTransform>(base_entity).scale = v3(prefab.scale);

    if (!prefab.drops.entries.empty())
        scene->registry.emplace<DropChance>(base_entity, prefab.drops);
    
    auto& model_comp = scene->registry.emplace<Model>(attachment_entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(prefab.attachment_model_path));
    model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);
    
    scene->registry.emplace<Caster>(attachment_entity, scene);
    scene->registry.emplace<Impairs>(attachment_entity);
    scene->registry.emplace<Attachment>(attachment_entity, base_entity);
    scene->registry.emplace<LogicTransform>(attachment_entity, v3(location));
    scene->registry.emplace<ModelTransform>(attachment_entity);
    if (model_comp.model_cpu->skeleton) {
        scene->registry.emplace<PoseController>(attachment_entity, *model_comp.model_cpu->skeleton);
    }
    
    Caster& caster = scene->registry.get<Caster>(attachment_entity);
    switch (prefab.type) {
        default: {
            caster.attack = std::make_unique<EnemyLaserAttack>(scene, attachment_entity, 0.5f, 1.0f, 1.5f, 1.0f);
            caster.attack->entry_gather_function = enemy_entry_gather();
        } break;
    }
    
    return base_entity;  
}

bool inspect(EnemyPrefab* enemy_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &enemy_prefab->file_path, "resources/enemies", FileType_Enemy);
    changed |= ImGui::EnumCombo("Type", &enemy_prefab->type);
    changed |= ImGui::PathSelect("Base Model", &enemy_prefab->base_model_path, "resources/models", FileType_Model);
    changed |= ImGui::PathSelect("Attachment Model", &enemy_prefab->attachment_model_path, "resources/models", FileType_Model);
    changed |= ImGui::PathSelect("Hurt", &enemy_prefab->hurt_path, "resources/emitters", FileType_Emitter);
    changed |= ImGui::DragFloat("Max Health", &enemy_prefab->max_health, 0.01f, 0.0f);
    changed |= ImGui::DragFloat("Max Speed", &enemy_prefab->max_speed, 0.01f, 0.0f);
    changed |= ImGui::DragFloat("Scale", &enemy_prefab->scale, 0.01f, 0.0f);
    changed |= inspect(&enemy_prefab->drops);
    
    return changed;
}

void enemy_ik_controller_system(Scene* scene) {
    for (auto [entity, logic_tfm, model_tfm, model, ik] : scene->registry.view<LogicTransform, ModelTransform, Model, SpiderController>().each()) {
        if (!model.model_cpu->skeleton)
            continue;
        
        model_tfm.translation = logic_tfm.position + v3(0.5f, 0.5f, 0.0f * ik.extra_z);
        model_tfm.rotation = math::quat_between(v3::Z, ik.quad_norm);
        model_tfm.dirty = true;

        m44 tfm = model_tfm.get_transform();
        m44 tfm_inv = math::inverse(tfm);

        ik.initialize_if_needed(tfm);
        ik.update_velocity(scene, logic_tfm);
        ik.update_desire(tfm);
        ik.resolve_desire();
        ik.update_target(scene, logic_tfm);
        assert_else(ik.check_all_targets_set());
        ik.update_transform_linkers();
        ik.update_constraints(scene, model, tfm_inv);

        if (scene->registry.any_of<Enemy>(entity)) {
            Enemy& enemy = scene->registry.get<Enemy>(entity);
            if (!scene->registry.valid(enemy.attachment))
                continue;

            LogicTransform& attachment_logic_tfm = scene->registry.get<LogicTransform>(enemy.attachment);
            attachment_logic_tfm.position = logic_tfm.position;

            ModelTransform& attachment_model_tfm = scene->registry.get<ModelTransform>(enemy.attachment);
            Bone* anchor_bone = model.model_cpu->skeleton->find_bone("anchor");
            if (anchor_bone) {
                m44 t =  tfm * model.model_cpu->root_node->cached_transform * anchor_bone->transform();
                attachment_model_tfm.set_translation(math::apply_transform(t, v3(0.0f, 0.0f, 0.0f)));
            }
        }
    }
}
void enemy_aggro_system(Scene* scene) {
    for (auto [entity, attachment, caster] : scene->registry.view<Attachment, Caster>().each()) {
        if (!caster.attack->has_target)
            continue;

        if (!scene->registry.valid(attachment.base))
            continue;
        
        Traveler* traveler = scene->registry.try_get<Traveler>(attachment.base);
        if (!traveler)
            continue;
        traveler->set_target({1, caster.attack->target});
    }

    for (auto [entity, enemy, traveler] : scene->registry.view<Enemy, Traveler>().each()) {
        if (!scene->registry.valid(enemy.target_consumer))
            continue;
        LogicTransform& consumer_tfm = scene->registry.get<LogicTransform>(enemy.target_consumer);
        traveler.set_target({2, math::round_cast(consumer_tfm.position)});
    }
}
void travel_system(Scene* scene) {
    for (auto [entity, transform, traveler] : scene->registry.view<LogicTransform, Traveler>().each()) {
        if (scene->registry.any_of<Impairs>(entity) && scene->registry.get<Impairs>(entity).is_impaired(scene, ImpairType_NoMove))
            continue;
        if (!traveler.has_target())
            continue;
        
        // Update pathing from target
        if (traveler.pathing.empty() || math::round_cast(traveler.pathing.front()) != traveler.target.pos) {
            traveler.pathing = scene->navigation->find_path(math::round_cast(transform.position), traveler.target.pos);
        }

        if (traveler.pathing.empty())
            continue;
        // if (scene->registry.any_of<Caster>(entity) && scene->registry.get<Caster>(entity).attack->casting())
        //     continue;
        
        // Use pathing to move
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

v3 predict_pos(Traveler& traveler, v3 pos, float time) {
    v3 expected_pos = pos;
    if (time == 0.0f)
        return expected_pos;
    for (u32 i = traveler.pathing.size(); i > 0; i--) {
        // TODO: If we're not using pathing, just return pos
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

float get_foot_height(Scene* scene, v3 enemy_origin, v2 foot_pos) {
    const umap<v3i, Direction>& ramps = scene->map_data.ramps;
    v3 out_pos;
    v3i out_cube;
    bool found = ray_intersection(scene->map_data.solids, ray3{v3(foot_pos, enemy_origin.z + 0.1f), v3(0.0f, 0.0f, -1.0f)}, out_pos, out_cube,
        [ramps](ray3 r, v3i v, v3& out_pos) {
            if (!ramps.contains(v))
                return false;

            switch (ramps.at(v)) {
                case Direction_PosX: {
                    float z_offset = math::fract(r.origin.x);
                    out_pos = v3(r.origin.xy, float(v.z-1) + z_offset);
                } break;
                case Direction_NegX: {
                    float z_offset = 1.0f - math::fract(r.origin.x);
                    out_pos = v3(r.origin.xy, float(v.z-1) + z_offset);
                } break;
                case Direction_PosY: {
                    float z_offset = math::fract(r.origin.y);
                    out_pos = v3(r.origin.xy, float(v.z-1) + z_offset);
                } break;
                case Direction_NegY: {
                    float z_offset = 1.0 - math::fract(r.origin.y);
                    out_pos = v3(r.origin.xy, float(v.z-1) + z_offset);
                } break;
            }
            
            return true;
        });

    if (found)
        return out_pos.z;
    else
        return enemy_origin.z;
}

void on_enemy_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    Enemy& enemy = registry.get<Enemy>(entity);
    if (!registry.valid(enemy.attachment))
        return;
    Attachment& attachment = registry.get<Attachment>(enemy.attachment);
    attachment.base = entt::null;
    if (attachment.requires_base)
        registry.emplace<Killed>(enemy.attachment);
}

}
