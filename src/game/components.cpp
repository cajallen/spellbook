#include "components.hpp"

#include <imgui.h>

#include "string.hpp"

#include "scene.hpp"

#include "renderer/renderable.hpp"
#include "renderer/draw_functions.hpp"


namespace spellbook {

void Model::inspect(Scene* scene) {

}
void Transform::inspect(Scene* scene) {
    ImGui::DragFloat3("Translation", translation.data, 0.01f);
    ImGui::DragFloat3("Rotation", rotation.data, 0.5f);
    ImGui::DragFloat("Scale", &scale, 0.01f);
}
void GridSlot::inspect(Scene* scene) {
    ImGui::DragInt3("Position", position.data, 0.01f);
    ImGui::Checkbox("Path", &path);
}
void Traveler::inspect(Scene* scene) {
    ImGui::DragFloat("Velocity", &velocity, 0.01f);
}
void Health::inspect(Scene* scene) {
    ImGui::DragFloat("Position", &value, 0.01f);
}
void Spawner::inspect(Scene* scene) {
    ImGui::DragFloat("Last Spawn", &last_spawn, 0.01f);
    ImGui::DragFloat("Spawn Rate", &rate, 0.01f);
}
void Consumer::inspect(Scene* scene) {
    ImGui::DragFloat("Consume Distance", &consume_distance, 0.01f);
}
void Killed::inspect(Scene* scene) {
    ImGui::DragFloat("When", &when, 0.01f);
}
void Pyro::inspect(Scene* scene) {
    ImGui::DragFloat("Radius", &radius, 0.01f);
    ImGui::DragFloat("Rate", &rate, 0.01f);
    ImGui::DragFloat("Damage", &damage, 0.05f);
}
void Roller::inspect(Scene* scene) {
    ImGui::DragFloat("Rate", &rate, 0.01f);
    ImGui::DragFloat("Damage", &damage, 0.05f);
    ImGui::DragFloat("Rollee Radius", &rollee_radius, 0.01f);
    ImGui::DragFloat("Rollee Speed", &rollee_speed, 0.05f);
    ImGui::DragFloat("Rollee Lifetime", &rollee_lifetime, 0.05f);
}
void Rollee::inspect(Scene* scene) {
    ImGui::DragFloat3("Velocity", velocity.data, 0.01f);
}
void Dragging::inspect(Scene* scene) {
    ImGui::DragFloat3("Start Position", start_position.data, 0.01f);
    ImGui::DragFloat3("Start Intersect", start_intersect.data, 0.01f);
}
void Collision::inspect(Scene* scene) {
    ImGui::DragFloat("Radius", &radius, 0.01f);
    if (ImGui::TreeNode("Colliding")) {
        for (auto ent : with) {
            scene->inspect_entity(ent);
        }
        ImGui::TreePop();
    }
}

void Model::preview_3d(Scene* scene, entt::entity entity) {}
void Transform::preview_3d(Scene* scene, entt::entity entity) {}
void GridSlot::preview_3d(Scene* scene, entt::entity entity) {}
void Traveler::preview_3d(Scene* scene, entt::entity entity) {}
void Health::preview_3d(Scene* scene, entt::entity entity) {}
void Spawner::preview_3d(Scene* scene, entt::entity entity) {}
void Consumer::preview_3d(Scene* scene, entt::entity entity) {}
void Killed::preview_3d(Scene* scene, entt::entity entity) {}
void Pyro::preview_3d(Scene* scene, entt::entity entity) {
    bool show;
    show = scene->registry.try_get<Dragging>(entity);
    if (!show)
        return;

    auto p_transform = scene->registry.try_get<Transform>(entity);
    if (p_transform) {
        vector<FormattedVertex> vertices;
        v3                      circ_pos = math::round(p_transform->translation);
        circ_pos.z                       = 0.03f;
        for (int i = 0; i <= 24; i++) {
            f32 angle  = i * math::TAU / 24.0f;
            v3  center = circ_pos + v3(0.5f, 0.5f, 0.0f);
            vertices.emplace_back(center + v3(radius * math::cos(angle), radius * math::sin(angle), 0.05f), palette::gold, 0.03f);
        }

        auto   line_mesh = generate_formatted_line(scene->render_scene.viewport.camera, std::move(vertices));
        // TODO:
    }
}
void Roller::preview_3d(Scene* scene, entt::entity entity) {
    bool show;
    show = scene->registry.try_get<Dragging>(entity);
    if (!show)
        return;

    auto p_transform = scene->registry.try_get<Transform>(entity);
    if (p_transform) {
        constexpr f32 preview_length = 2.0f;

        v3 center = v3(math::round(p_transform->translation).xy, 0.0f) + v3(0.5f, 0.5f, 0.03f);

        vector<FormattedVertex> vertices;
        vertices.emplace_back(center + v3(preview_length, rollee_radius, 0.0f), palette::slate_gray, 0.02f);
        vertices.emplace_back(center + v3(rollee_radius, rollee_radius, 0.0f), palette::slate_gray, 0.03f);
        vertices.emplace_back(center + v3(rollee_radius, preview_length, 0.0f), palette::slate_gray, 0.02f);
        vertices.emplace_back(center + v3(rollee_radius, preview_length + 0.01f, 0.0f), palette::slate_gray, 0.00f);

        vertices.emplace_back(center + v3(-rollee_radius, preview_length + 0.01f, 0.0f), palette::slate_gray, 0.00f);
        vertices.emplace_back(center + v3(-rollee_radius, preview_length, 0.0f), palette::slate_gray, 0.02f);
        vertices.emplace_back(center + v3(-rollee_radius, rollee_radius, 0.0f), palette::slate_gray, 0.03f);
        vertices.emplace_back(center + v3(-preview_length, rollee_radius, 0.0f), palette::slate_gray, 0.02f);
        vertices.emplace_back(center + v3(-preview_length - 0.01f, rollee_radius, 0.0f), palette::slate_gray, 0.00f);

        vertices.emplace_back(center + v3(-preview_length - 0.01f, -rollee_radius, 0.0f), palette::slate_gray, 0.00f);
        vertices.emplace_back(center + v3(-preview_length, -rollee_radius, 0.0f), palette::slate_gray, 0.02f);
        vertices.emplace_back(center + v3(-rollee_radius, -rollee_radius, 0.0f), palette::slate_gray, 0.03f);
        vertices.emplace_back(center + v3(-rollee_radius, -preview_length, 0.0f), palette::slate_gray, 0.02f);
        vertices.emplace_back(center + v3(-rollee_radius, -preview_length - 0.01f, 0.0f), palette::slate_gray, 0.00f);

        vertices.emplace_back(center + v3(rollee_radius, -preview_length - 0.01f, 0.0f), palette::slate_gray, 0.00f);
        vertices.emplace_back(center + v3(rollee_radius, -preview_length, 0.0f), palette::slate_gray, 0.02f);
        vertices.emplace_back(center + v3(rollee_radius, -rollee_radius, 0.0f), palette::slate_gray, 0.03f);
        vertices.emplace_back(center + v3(preview_length, -rollee_radius, 0.0f), palette::slate_gray, 0.02f);

        auto line_mesh = generate_formatted_line(scene->render_scene.viewport.camera, std::move(vertices));
        // TODO:
    }
}
void Rollee::preview_3d(Scene* scene, entt::entity entity) {}

void Dragging::preview_3d(Scene* scene, entt::entity entity) {
    bool show;
    show = scene->registry.try_get<Dragging>(entity);
    if (!show)
        return;

    auto p_transform = scene->registry.try_get<Transform>(entity);
    if (p_transform) {
        vector<FormattedVertex> vertices;
        v3                      circ_pos = math::round(p_transform->translation);
        circ_pos.z                       = 0.03f;
        for (int i = 0; i <= 24; i++) {
            f32 angle  = i * math::TAU / 24.0f;
            v3  center = circ_pos + v3(0.5f, 0.5f, 0.0f);
            vertices.emplace_back(center + 0.5f * v3(math::cos(angle), math::sin(angle), 0.05f), palette::white, 0.03f);
        }

        auto   line_mesh = generate_formatted_line(scene->render_scene.viewport.camera, std::move(vertices));
        // TODO:
    }
}
void Collision::preview_3d(Scene* scene, entt::entity entity) {}

}

