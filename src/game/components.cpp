#include "components.hpp"

#include <imgui.h>

#include "lib/string.hpp"
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
            ImGui::Text("Skeleton");
            ImGui::Indent();
            bool any_changed = false;

            if (ImGui::BeginTable("bones", 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Id");
                ImGui::TableSetupColumn("Parent");
                ImGui::TableSetupColumn("Position");
                ImGui::TableSetupColumn("Rotation");
                ImGui::TableSetupColumn("Scale");
                ImGui::TableHeadersRow();
                for (id_ptr<Bone> bone : skeleton->bones) {
                    ImGui::PushID(bone.id);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", bone.id);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", bone->parent.id);
                    
                    ImGui::TableSetColumnIndex(2);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    any_changed |= ImGui::DragFloat3("##Position", bone->positions.front().position.data, 0.01f);               
                    ImGui::TableSetColumnIndex(3);
                    euler e = math::to_euler(bone->rotations.front().orientation);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if (ImGui::DragFloat3("##Rotation", e.data, 0.01f)) {
                        bone->rotations.front().orientation = math::to_quat(e);
                        any_changed = true;
                    }
                    ImGui::TableSetColumnIndex(4);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    any_changed |= ImGui::DragFloat3("##Scale", bone->scales.front().scale.data, 0.01f);
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }

            ImGui::Checkbox("Render Lines", &skeleton->render_lines);
            
            if (skeleton->render_lines) {
                uset<u64> is_parent;
                for (u32 i = 0; i < skeleton->bones.size(); i++) {
                    if (skeleton->bones[i]->parent.valid())
                        is_parent.insert(skeleton->bones[i]->parent.id);
                }
                for (id_ptr<Bone> bone : skeleton->bones) {
                    if (!is_parent.contains(bone.id)) {
                        vector<FormattedVertex> vertices;
                        id_ptr<Bone> current_node = bone;
                        vertices.push_back({current_node->positions.front().position, palette::white, 0.03f});
                        while (current_node->parent.valid()) {
                            current_node = current_node->parent;
                            vertices.push_back({current_node->positions.front().position, palette::white, 0.03f});
                        }
                        auto line_mesh = generate_formatted_line(scene->render_scene.viewport.camera, std::move(vertices));
                        if (line_mesh.file_path.empty())
                            continue;
                        scene->render_scene.quick_mesh(line_mesh);
                    }
                }
                
            }

            if (any_changed)
                skeleton->update();


            ImGui::Unindent();


            
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

        render_scene.quick_mesh(generate_formatted_line(render_scene.viewport.camera, std::move(vertices)));
    }
}

}
