#include "components.hpp"

#include <imgui.h>
#include <tracy/Tracy.hpp>

#include "extension/imgui_extra.hpp"
#include "general/string.hpp"
#include "general/matrix_math.hpp"
#include "game/scene.hpp"
#include "game/spawner.hpp"
#include "game/lizard.hpp"
#include "game/pose_controller.hpp"
#include "renderer/draw_functions.hpp"


namespace spellbook {

void          inspect_components(Scene* scene, entt::entity entity) {
    if (auto* component = scene->registry.try_get<Model>(entity)) {
        ZoneScoped;
        ImGui::Text("Model");
        if (scene->registry.all_of<ModelTransform>(entity)) {
            auto transform = scene->registry.get<ModelTransform>(entity);
            inspect(&*component->model_cpu, transform.transform, &scene->render_scene);
        }
        else {
            inspect(&*component->model_cpu, m44::identity(), &scene->render_scene);
        }
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<LogicTransform>(entity)) {
        ImGui::Text("LogicTransform");
        ImGui::DragFloat3("Position", component->position.data, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<ModelTransform>(entity)) {
        ImGui::Text("ModelTransform");
        ImGui::DragFloat3("Translation", component->translation.data, 0.01f);
        ImGui::DragFloat3("Rotation", component->rotation.data, 0.5f);
        ImGui::DragFloat3("Scale", component->scale.data, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<TransformLink>(entity)) {
        ImGui::Text("TransformLink");
        ImGui::DragFloat3("Offset", component->offset.data, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<LogicTransformAttach>(entity)) {
        ImGui::Text("LogicTransformAttach");
        ImGui::DragFloat3("Offset", component->offset.data, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<GridSlot>(entity)) {
        ImGui::Text("GridSlot");
        ImGui::Checkbox("Path", &component->path);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Traveler>(entity)) {
        ImGui::Text("Traveler");
        if (ImGui::TreeNode("Max Speed")) {
            inspect(&component->max_speed);
            ImGui::TreePop();
        }
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Health>(entity)) {
        ImGui::Text("Health");
        ImGui::DragFloat("Value", &component->value, 0.01f);
        if (ImGui::TreeNode("Max Health")) {
            inspect(&component->max_health);
            ImGui::TreePop();
        }
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Consumer>(entity)) {
        ImGui::Text("Consumer");
        ImGui::DragFloat("Consume Distance", &component->consume_distance, 0.01f);
        ImGui::Text("Amount Consumed: %d", component->amount_consumed);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Killed>(entity)) {
        ImGui::Text("Killed");
        ImGui::DragFloat("When", &component->when, 0.01f);
        ImGui::Separator();
    }
    if (scene->registry.all_of<Draggable>(entity)) {
        ImGui::Text("Draggable");
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Dragging>(entity)) {
        ImGui::Text("Dragging");
        ImGui::DragFloat("Start Time", &component->start_time, 0.01f);
        ImGui::DragFloat3("Start Logic Position", component->start_logic_position.data, 0.01f);
        ImGui::DragFloat3("Start Intersect", component->start_intersect.data, 0.01f);
        ImGui::DragFloat3("Target Position", component->target_position.data, 0.01f);
        ImGui::DragFloat3("Potential Position", component->potential_logic_position.data, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Collision>(entity)) {
        ImGui::Text("Collision");
        ImGui::DragFloat("Radius", &component->radius, 0.01f);
        ImGui::Text("Colliding");
        for (auto ent : component->with) {
            scene->inspect_entity(ent);
        }
        ImGui::TreePop();
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Spawner>(entity)) {
        ImGui::Text("Spawner");
        ImGui::Text("Delta Cost Stat");
        inspect(&component->delta_cost);
        ImGui::Separator();
        ImGui::Checkbox("Wave Happening", &component->wave_happening);
        ImGui::DragFloat("Cost Total", &component->cost_total, 0.01f);
        ImGui::DragFloat("Cooldown", &component->cooldown, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<PoseController>(entity)) {
        ZoneScoped;
        ImGui::Text("Poser");
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Lizard>(entity)) {
        ZoneScoped;
        ImGui::Text("Lizard");
        ImGui::EnumCombo("Type", &component->type);
        if (ImGui::TreeNode("Basic Ability")) {
            inspect(&*component->basic_ability);
            ImGui::TreePop();
        }
        ImGui::Separator();
    }
}

void preview_3d_components(Scene* scene, entt::entity entity) {
    RenderScene& render_scene = scene->render_scene;

    bool show;
    show = scene->registry.try_get<Dragging>(entity);
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
    if (rotation != e) {
        rotation = e;
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

}
