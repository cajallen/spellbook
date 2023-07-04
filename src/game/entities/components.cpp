#include "components.hpp"

#include <cinttypes>
#include <imgui.h>
#include <tracy/Tracy.hpp>

#include "caster.hpp"
#include "impair.hpp"
#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "game/game.hpp"
#include "general/string.hpp"
#include "general/matrix_math.hpp"
#include "renderer/draw_functions.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/tile.hpp"
#include "game/entities/spawner.hpp"
#include "game/entities/consumer.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/enemy_ik.hpp"


namespace spellbook {

void remove_dragging_impair(Scene* scene, entt::entity entity) {
    remove_dragging_impair(scene->registry, entity);
}

void remove_dragging_impair(entt::registry& reg, entt::entity entity) {
    auto impairs = reg.try_get<Impairs>(entity);
    if (impairs) {
        impairs->untimed_impairs.erase(Dragging::magic_number | u32(entity));
    }
}

// ModelTransform::ModelTransform() {
//     data = std::make_unique<ModelTransformData>();
// }
// ModelTransform::ModelTransform(v3 translation, euler rotation, v3 scale) {
//     data = std::make_unique<ModelTransformData>(translation, rotation, scale);
// }

#define INSPECT_COMPONENT(Comp, body) if (Comp* component = scene->registry.try_get<Comp>(entity)) { \
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);\
    if (ImGui::CollapsingHeader(#Comp)) { \
        body\
    }\
}

void inspect_components(Scene* scene, entt::entity entity) {
    INSPECT_COMPONENT(Model,
        inspect(&*component->model_cpu, &scene->render_scene);
    )
    INSPECT_COMPONENT(LogicTransform,
        ImGui::DragFloat3("Position", component->position.data, 0.01f);
        ImGui::DragFloat3("Normal", component->normal.data, 0.01f);
        ImGui::DragFloat("Yaw", &component->yaw, 0.01f);
    )
    INSPECT_COMPONENT(ModelTransform,
        bool changed = ImGui::DragFloat3("Translation", component->translation.data, 0.01f);
        changed |= ImGui::DragFloat3("Scale", component->scale.data, 0.01f);
        if (changed)
            component->dirty = true;
    )
    INSPECT_COMPONENT(TransformLink, 
        ImGui::DragFloat3("Offset", component->offset.data, 0.01f);
    )
    INSPECT_COMPONENT(Enemy, 
        ImGui::Text("attachment: %s", scene->registry.valid(component->attachment) ? scene->registry.get<Name>(component->attachment).name.c_str() : "none");
        ImGui::Text("spawner: %s", scene->registry.valid(component->from_spawner) ? scene->registry.get<Name>(component->from_spawner).name.c_str() : "none");
        ImGui::Text("consumer: %s", scene->registry.valid(component->target_consumer) ? scene->registry.get<Name>(component->target_consumer).name.c_str() : "none");
    )
    INSPECT_COMPONENT(Health,
        ImGui::DragFloat("Value", &component->value, 0.01f);
        if (ImGui::TreeNode("Max Health")) {
            inspect(&*component->max_health);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Damage Taken")) {
            inspect(&*component->damage_taken_multiplier);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Regen")) {
            inspect(&*component->regen);
            ImGui::TreePop();
        }
    )
    INSPECT_COMPONENT(Shrine,
        ImGui::Checkbox("Egg Attached", &component->egg_attached);
    )
    INSPECT_COMPONENT(Killed, 
        ImGui::DragFloat("When", &component->when, 0.01f);
    )
    INSPECT_COMPONENT(Collision, 
        ImGui::DragFloat("Radius", &component->radius, 0.01f);
        ImGui::Text("Colliding");
        for (auto ent : component->with) {
            scene->inspect_entity(ent);
        }
        ImGui::TreePop();
    )
    INSPECT_COMPONENT(Spawner, 
        inspect(component);
    )
    INSPECT_COMPONENT(Traveler, 
        inspect(component);
    )
    INSPECT_COMPONENT(PoseController,
        ImGui::EnumCombo("State", &component->state);
        ImGui::Text("Pose set: %16" PRIXPTR, component->animation);
        ImGui::Text("Target index: %d", component->target_index);
        ImGui::Text("Fractional state total: %.2f", component->fractional_state_total);
        ImGui::Text("Time to override: %.2f", component->time_to_override);
    )
    INSPECT_COMPONENT(Lizard,
        ImGui::EnumCombo("Type", &component->type);
    )
    INSPECT_COMPONENT(Caster,
        ImGui::Text("Attack");
        inspect(&*component->attack);
        ImGui::Text("Spell");
        inspect(&*component->spell);
    )
    INSPECT_COMPONENT(SpiderController,
        component->inspect();
    )
    ImGui::Separator();
}

void preview_3d_components(Scene* scene, entt::entity entity) {
    RenderScene& render_scene = scene->render_scene;

    bool show;
    show = scene->registry.any_of<Dragging>(entity);
    if (!show)
        return;
    
    if (auto* component = scene->registry.try_get<Dragging>(entity)) {
        vector<FormattedVertex> vertices;

        v3 center = component->potential_logic_position + v3(0.5f, 0.5f, 0.05f);
        for (int i = 0; i <= 32; i++) {
            constexpr f32 radius = 0.5f;
            f32 angle  = i * math::TAU / 32.0f;
            vertices.emplace_back(center + radius * v3(math::cos(angle), math::sin(angle), 0.0f), palette::white, 0.02f);
        }

        render_scene.quick_mesh(generate_formatted_line(render_scene.viewport.camera, std::move(vertices)), true, false);
    }
}


void ModelTransform::set_translation(const v3& v) {
    if (translation != v) {
        translation = v;
        dirty = true;
    }
}
void ModelTransform::set_rotation(const euler& e) {
    quat q = math::to_quat(e);
    if (math::dot(rotation, q) <= 0.9999f) {
        rotation = q;
        dirty = true;
    }
}

void ModelTransform::set_rotation(const quat& q) {
    float len = math::length(rotation);
    assert_else(1.00001 >= len && len >= 0.99999f)
        rotation /= len;
    if (math::dot(rotation, q) <= 0.99999f) {
        rotation = q;
        dirty = true;
    }
}

void ModelTransform::set_scale(const v3& v) {
    if (scale != v) {
        scale = v;
        dirty = true;
    }
}

const m44& ModelTransform::get_transform() {
    if (dirty) {
        dirty = false;
        transform = math::translate(translation) * math::rotation(rotation) * math::scale(scale);
    }
    return transform;
}

Health::Health(float health_value, Scene* init_scene, const string& hurt_emitter_path) {
    scene = init_scene;
    value = health_value;
    buffer_value = value;
    max_health = std::make_unique<Stat>(scene, value);
    damage_taken_multiplier = std::make_unique<Stat>(scene, 1.0f);
    emitter_cpu_path = hurt_emitter_path;

}

void damage(Scene* scene, entt::entity damager, entt::entity damagee, float amount, v3 direction) {
    Caster* caster = scene->registry.try_get<Caster>(damager);
    Health& health = scene->registry.get<Health>(damagee);

    health.damage_signal.publish(scene, damager, damagee, amount);
    
    float hurt_value = (caster ? stat_instance_value(&*caster->damage, amount) : amount) * health.damage_taken_multiplier->value();
    health.value -= hurt_value;

    if (!health.emitter_cpu_path.empty()) {
        EmitterCPU hurt_emitter = load_asset<EmitterCPU>(health.emitter_cpu_path);
        hurt_emitter.rotation = math::quat_between(v3(1,0,0), math::normalize(direction));
        u64 random_id = math::random_u64();
        EmitterComponent& emitter = scene->registry.get<EmitterComponent>(damagee);
        emitter.add_emitter(random_id, hurt_emitter);

        float hurt_time = math::map_range(hurt_value / health.max_health->value(), {0.0f, 1.0f}, {0.1f, 0.3f});
        add_timer(scene, fmt_("hurt_timer"), [scene, damagee, random_id](Timer* timer) {
            if (scene->registry.valid(damagee))
                scene->registry.get<EmitterComponent>(damagee).remove_emitter(random_id);
        }, true)->start(hurt_time);
    }
}

void EmitterComponent::add_emitter(u64 id, const EmitterCPU& emitter_cpu) {
    emitters[id] = &instance_emitter(scene->render_scene, emitter_cpu);
}

void EmitterComponent::remove_emitter(u64 id) {
    if (!emitters.contains(id))
        return;
    deinstance_emitter(*emitters[id], true);
    emitters.erase(id);
}

entt::entity setup_basic_unit(Scene* scene, const string& model_path, v3 location, float health_value, const string& hurt_path) {
    auto entity = scene->registry.create();
    scene->registry.emplace<AddToInspect>(entity);

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(model_path));
    model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f, 0.5f, 0.0f));
    
    scene->registry.emplace<EmitterComponent>(entity, scene);
    scene->registry.emplace<Health>(entity, health_value, scene, hurt_path);
    scene->registry.emplace<Impairs>(entity);
    
    if (model_comp.model_cpu->skeleton) {
        scene->registry.emplace<PoseController>(entity, *model_comp.model_cpu->skeleton);
    }

    return entity;
}


void on_dragging_create(Scene& scene, entt::registry& registry, entt::entity entity) {
    auto impairs = registry.try_get<Impairs>(entity);
    if (impairs) {
        apply_untimed_impair(*impairs, Dragging::magic_number | u32(entity), ImpairType_NoCast);
    }

    Caster* caster = registry.try_get<Caster>(entity);
    if (caster && caster->casting()) {
        caster->attack->stop_casting();
        caster->spell->stop_casting();
    }
}

void on_dragging_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    auto& dragging = registry.get<Dragging>(entity);
    auto& logic_tfm = registry.get<LogicTransform>(entity);
    
    scene.audio.play_sound("audio/step.flac", {.position = logic_tfm.position});
    
    if (registry.any_of<Egg>(entity)) {
        entt::entity shrine_entity = scene.get_shrine(math::round_cast(dragging.potential_logic_position));
        Shrine* shrine = registry.try_get<Shrine>(shrine_entity);
        if (!shrine || shrine->egg_entity != entity)
            return;
        shrine->egg_attached = true;
    }
    
    if (math::length(logic_tfm.position - math::round(dragging.potential_logic_position)) > 0.1f)
        scene.player.bank.beads[Bead_Quartz]--;
    
    logic_tfm.position = math::round(dragging.potential_logic_position);

    constexpr float drag_fade_duration = 0.15f;
    
    auto poser = registry.try_get<PoseController>(entity);
    if (poser) {
        poser->set_state(AnimationState_Idle, 0.0f, drag_fade_duration);
    }

    bool remain_impaired = false;
    if (scene.is_casting_platform(math::round_cast(dragging.potential_logic_position))) {
        auto caster = registry.try_get<Caster>(entity);
        if (caster) {
            Ability* ability = &*caster->spell;
            add_timer(caster->spell->scene, "drag_cast_delay",
                [ability](Timer* timer) {
                    ability->targeting();
                    ability->request_cast();
                }, true
            )->start(drag_fade_duration);
            remain_impaired = true;
        }
    }

    if (!remain_impaired) {
        remove_dragging_impair(registry, entity);
    }
}


void on_model_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    Model& model = registry.get<Model>(entity);
    deinstance_model(scene.render_scene, model.model_gpu);
}


void on_emitter_component_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    auto& emitter = registry.get<EmitterComponent>(entity);
    for (auto& [id, emitter_gpu] : emitter.emitters)
        deinstance_emitter(*emitter_gpu, true);
}

void on_gridslot_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    GridSlot* grid_slot = registry.try_get<GridSlot>(entity);
    if (!grid_slot)
        return;
    for (entt::entity neighbor : grid_slot->linked) {
        registry.get<GridSlot>(neighbor).linked.clear();
        if (registry.valid(neighbor))
            registry.destroy(neighbor);
    }
}

}
