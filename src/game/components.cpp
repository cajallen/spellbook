#include "components.hpp"

#include <imgui.h>

#include "general/string.hpp"
#include "general/matrix_math.hpp"
#include "game/scene.hpp"
#include "game/spawner.hpp"
#include "game/tower.hpp"
#include "renderer/draw_functions.hpp"


namespace spellbook {

void          inspect_components(Scene* scene, entt::entity entity) {
    if (auto* component = scene->registry.try_get<ModelTransform>(entity)) {
        ImGui::Text("ModelTransform");
        ImGui::DragFloat3("Translation", component->translation.data, 0.01f);
        ImGui::DragFloat3("Rotation", component->rotation.data, 0.5f);
        ImGui::DragFloat("Scale", &component->scale, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<GridSlot>(entity)) {
        ImGui::Text("GridSlot");
        ImGui::Checkbox("Path", &component->path);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Health>(entity)) {
        ImGui::Text("Health");
        ImGui::DragFloat("Position", &component->value, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Consumer>(entity)) {
        ImGui::Text("Consumer");
        ImGui::DragFloat("Consume Distance", &component->consume_distance, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Killed>(entity)) {
        ImGui::Text("Killed");
        ImGui::DragFloat("When", &component->when, 0.01f);
        ImGui::Separator();
    }
    if (auto* component = scene->registry.try_get<Dragging>(entity)) {
        ImGui::Text("Dragging");
        ImGui::DragFloat3("Start Logic Position", component->start_logic_position.data, 0.01f);
        ImGui::DragFloat3("Start Intersect", component->start_intersect.data, 0.01f);
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
    if (auto* component = scene->registry.try_get<Model>(entity)) {
        ImGui::Text("Model");
        for (auto&& skeleton : component->model_gpu.skeletons) {
            inspect(skeleton, &scene->render_scene);
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

        v3 circ_pos = math::round(component->logic_position);
        circ_pos.z  = 0.03f;
        for (int i = 0; i <= 24; i++) {
            f32 angle  = i * math::TAU / 24.0f;
            v3  center = circ_pos + v3(0.5f, 0.5f, 0.0f);
            vertices.emplace_back(center + 0.5f * v3(math::cos(angle), math::sin(angle), 0.05f), palette::white, 0.03f);
        }

        render_scene.quick_mesh(generate_formatted_line(render_scene.viewport.camera, std::move(vertices)), true);
    }
}

}
