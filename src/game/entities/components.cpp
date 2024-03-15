#include "components.hpp"

#include <imgui.h>
#include <tracy/Tracy.hpp>
#include <entt/core/hashed_string.hpp>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/math/matrix_math.hpp"
#include "renderer/draw_functions.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/tags.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/spawner.hpp"
#include "game/entities/consumer.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/enemy_ik.hpp"


namespace spellbook {

using namespace entt::literals;

void remove_dragging_impair(Scene* scene, entt::entity entity) {
    remove_dragging_impair(scene->registry, entity);
}

void remove_dragging_impair(entt::registry& reg, entt::entity entity) {
    Tags* tags = reg.try_get<Tags>(entity);
    if (tags) {
        tags->remove_tag(Dragging::get_drag_tag_id(entity));
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
            component->set_dirty();
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
            constexpr float radius = 0.5f;
            float angle  = i * math::TAU / 32.0f;
            vertices.emplace_back(center + radius * v3(math::cos(angle), math::sin(angle), 0.0f), palette::white, 0.02f);
        }

        render_scene.quick_mesh(generate_formatted_line(render_scene.viewport.camera, std::move(vertices)), true, false);
    }
}


void ModelTransform::set_translation(const v3& v) {
    if (translation != v) {
        translation = v;
        set_dirty();
    }
}

void ModelTransform::set_rotation(const quat& q) {
    float len = math::length(rotation);
    assert_else(1.00001 >= len && len >= 0.99999f)
        rotation /= len;
    if (math::dot(rotation, q) <= 0.99999f) {
        rotation = q;
        set_dirty();
    }
}

void ModelTransform::set_scale(const v3& v) {
    if (scale != v) {
        scale = v;
        set_dirty();
    }
}

void ModelTransform::set_dirty() {
    transform_dirty = true;
    renderable_dirty = true;
}

const m44& ModelTransform::get_transform() {
    if (transform_dirty) {
        transform_dirty = false;
        transform = math::translate(translation) * math::rotation(rotation) * math::scale(scale);
    }
    return transform;
}

Health::Health(float health_value, Scene* init_scene, entt::entity init_entity, const FilePath& hurt_emitter_path) : scene(init_scene), entity(init_entity) {
    value = health_value;
    buffer_value = value;
    max_health = std::make_unique<Stat>(scene, entity, value);
    damage_taken_multiplier = std::make_unique<Stat>(scene, entity, 1.0f);
    emitter_cpu_path = hurt_emitter_path;
}

void Health::damage(entt::entity damager, float amount, v3 direction) {
    damage_signal.publish(scene, damager, entity, amount);

    Caster* caster = scene->registry.try_get<Caster>(damager);
    float hurt_value = (caster ? stat_instance_value(&*caster->damage, amount) : amount) * damage_taken_multiplier->value();
    value -= hurt_value;

    if (emitter_cpu_path.is_file()) {
        EmitterCPU hurt_emitter = load_resource<EmitterCPU>(emitter_cpu_path);
        hurt_emitter.rotation = math::quat_between(v3(1,0,0), math::normalize(direction));
        uint64 random_id = math::random_uint64();
        EmitterComponent& emitter = scene->registry.get<EmitterComponent>(entity);
        emitter.add_emitter(random_id, hurt_emitter);

        float hurt_time = math::map_range(hurt_value / max_health->value(), {0.0f, 1.0f}, {0.1f, 0.3f});
        add_timer(scene, [random_id, this](Timer* timer) {
            if (this->scene->registry.valid(entity))
                this->scene->registry.get<EmitterComponent>(entity).remove_emitter(random_id);
        }, true)->start(hurt_time);
    }
}

void Health::apply_dot(entt::entity damager, uint64 id, const StatEffect& effect, EmitterCPU* emitter) {
    if (!dots.contains(damager))
        dots[damager] = std::make_unique<Stat>(scene, entity, 0.0f);
    // different id
    dots[damager]->add_effect(1, effect, emitter);
}


void EmitterComponent::add_emitter(uint64 id, const EmitterCPU& emitter_cpu) {
    emitters[id] = &instance_emitter(scene->render_scene, emitter_cpu, Input::time);
}

void EmitterComponent::remove_emitter(uint64 id) {
    if (!emitters.contains(id))
        return;
    deinstance_emitter(*emitters[id], true);
    emitters.erase(id);
}

entt::entity setup_basic_unit(Scene* scene, const FilePath& model_path, v3 location, float health_value, const FilePath& hurt_path) {
    auto entity = scene->registry.create();
    scene->registry.emplace<AddToInspect>(entity);

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_resource<ModelCPU>(model_path));
    model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<Grounded>(entity);
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f, 0.5f, 0.0f));
    
    scene->registry.emplace<EmitterComponent>(entity, scene);
    scene->registry.emplace<Health>(entity, health_value, scene, entity, hurt_path);
    scene->registry.emplace<Tags>(entity, *scene);
    
    if (model_comp.model_cpu->skeleton) {
        scene->registry.emplace<PoseController>(entity, *model_comp.model_cpu->skeleton);
    }

    return entity;
}


void on_gridslot_create(Scene& scene, entt::registry& registry, entt::entity entity) {
    scene.map_data.dirty = true;
}

void on_gridslot_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    scene.map_data.dirty = true;

    GridSlot* grid_slot = registry.try_get<GridSlot>(entity);
    if (!grid_slot)
        return;
    for (entt::entity neighbor : grid_slot->linked) {
        registry.get<GridSlot>(neighbor).linked.clear();
        if (registry.valid(neighbor))
            registry.destroy(neighbor);
    }
}

void on_dragging_create(Scene& scene, entt::registry& registry, entt::entity entity) {
    auto tags = registry.try_get<Tags>(entity);
    if (tags) {
        tags->apply_tag("no_cast"_hs, Dragging::get_drag_tag_id(entity));
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

    // we can't rely on select lizard, because the mapping breaks down while dragging
    entt::entity potential_swap = entt::null;
    for (auto [potential_entity, potential_lizard, potential_logic_tfm] : scene.registry.view<Lizard, LogicTransform>().each()) {
        if (entity == potential_entity)
            continue;
        if (math::distance(dragging.potential_logic_position, potential_logic_tfm.position) < 0.1f)
            potential_swap = potential_entity;
    }
    if (scene.registry.valid(potential_swap)) {
        scene.registry.emplace<ForceDragging>(potential_swap, dragging.start_logic_position, 0.2f);
    }

    scene.audio.play_sound("audio/step.flac"_resource, {.position = logic_tfm.position});
    
    if (registry.any_of<Egg>(entity)) {
        entt::entity shrine_entity = scene.get_shrine(math::round_cast(dragging.potential_logic_position));
        Shrine* shrine = registry.try_get<Shrine>(shrine_entity);
        if (!shrine || shrine->egg_entity != entity)
            return;
        shrine->egg_attached = true;
    }
    
    if (!registry.any_of<ForceDragging>(entity) && math::length(logic_tfm.position - math::round(dragging.potential_logic_position)) > 0.1f)
        if (dragging.commited_cost.amount > 0)
            scene.player.bank.beads[dragging.commited_cost.type] -= dragging.commited_cost.amount;
    
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
            add_timer(caster->spell->scene,
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

void on_static_model_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    StaticModel& model = registry.get<StaticModel>(entity);
    deinstance_static_model(scene.render_scene, model.renderables);
}


void on_emitter_component_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    auto& emitter = registry.get<EmitterComponent>(entity);
    for (auto& [id, emitter_gpu] : emitter.emitters)
        deinstance_emitter(*emitter_gpu, true);
}

void on_forcedrag_create(Scene& scene, entt::registry& registry, entt::entity entity) {
    if (registry.all_of<Draggable>(entity)) {
        Draggable& draggable = registry.get<Draggable>(entity);
        LogicTransform& logic_tfm = registry.get<LogicTransform>(entity);
        registry.emplace<Dragging>(entity, Beads{Bead_Quartz, 0}, draggable.drag_height, scene.time, logic_tfm.position);
    } else {
        log_error("Force Drag on undraggable");
    }
}

void on_forcedrag_destroy(Scene& scene, entt::registry& registry, entt::entity entity) {
    registry.remove<Dragging>(entity);
}

}
