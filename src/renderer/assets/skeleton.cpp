#include "skeleton.hpp"

#include <vuk/Buffer.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "editor/pose_widget.hpp"
#include "extension/fmt.hpp"
#include "general/matrix_math.hpp"
#include "general/logger.hpp"
#include "game/input.hpp"
#include "game/game.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"


namespace spellbook {

void SkeletonGPU::save_pose(string name) {
    if (!poses.contains(name)) {
        poses[name] = vector<KeySet>();
        poses[name].resize(bones.size());
    }
    
    for (u32 i = 0; i < bones.size(); i++) {
        poses[name][i] = bones[i]->start;
    }
}

void SkeletonGPU::load_pose(string name, bool as_target, float offset) {
    if (as_target)
        mode = Mode_Play;

    for (u32 i = 0; i < bones.size(); i++) {
        if (!as_target)
            bones[i]->start = poses[name][i];
        else {
            bones[i]->target = poses[name][i];
            bones[i]->target.position.time = Input::time + offset;
            bones[i]->target.rotation.time = Input::time + offset;
            bones[i]->target.scale.time = Input::time + offset;
            bones[i]->start.position.time = Input::time;
            bones[i]->start.rotation.time = Input::time;
            bones[i]->start.scale.time = Input::time;
        }
    }
}

void SkeletonGPU::stop_playing() {
    for (u32 i = 0; i < bones.size(); i++) {
        if (mode == Mode_Play)
            bones[i]->start = bones[i]->target;
    }
    mode = Mode_Pose;
}


m44 Bone::transform() const {
    if (parent.valid())
        return parent->transform() * local_transform;
    return local_transform;
}

m44 Bone::final_transform() const {
    if (parent.valid())
        return parent->transform() * local_transform * inverse_bind_matrix;
    return local_transform * inverse_bind_matrix;
}

void Bone::update(float new_time) {
    if (new_time == -1.0f)
        time = -1.0f;
    else
        time = new_time;
    
    m44 translation = update_position();
    m44 rotation    = update_rotation();
    m44 scale       = update_scaling();
    local_transform  = translation * rotation * scale;
}

m44 Bone::update_position() {
    if (time == -1.0f)
        return math::translate(start.position.value);
    
    float t = math::from_range(time, range(start.position.time, target.position.time));
    t = math::abs(1.0f - math::mod(t, 2.0f));
    v3 interpolated = math::mix(start.position.value, target.position.value, math::ease(t, ease_mode));
    return math::translate(interpolated);
}

m44 Bone::update_rotation() {
    if (time == -1.0f)
        return math::rotation(start.rotation.value);

    float t = math::from_range(time, range(start.rotation.time, target.rotation.time));
    t = math::abs(1.0f - math::mod(t, 2.0f));
    quat interpolated = math::slerp(start.rotation.value, target.rotation.value, math::ease(t, ease_mode));
    return math::rotation(interpolated);
}

m44 Bone::update_scaling() {
    if (time == -1.0f)
        return math::scale(start.scale.value);

    float t = math::from_range(time, range(start.scale.time, target.scale.time));
    t = math::abs(1.0f - math::mod(t, 2.0f));
    v3 interpolated = math::mix(start.scale.value, target.scale.value, math::ease(t, ease_mode));
    return math::scale(interpolated);
}

void SkeletonGPU::update() {
    for (auto bone_ptr : bones) {
        switch (mode) {
            case (Mode_Pose): {
                bone_ptr->update(-1.0f);
            } break;
            case (Mode_Play): {
                bone_ptr->update(Input::time);
            } break;
            default: {
                log_error("Skeleton update mode NYI");
            }
        }
    }

    vector<u8> bones_data( sizeof(u32) * 4 + sizeof(m44GPU) * bones.size());
    bones_data.append_data(bones.size());
    struct { u32 a,b,c; } padding;
    bones_data.append_data(padding);
    for (id_ptr<Bone> bone : bones) {
        bones_data.append_data((m44GPU) bone->final_transform());
    }
    memcpy(buffer->mapped_ptr, bones_data.data(), bones_data.size());
}

vuk::Unique<vuk::Buffer>* SkeletonGPU::empty_buffer() {
    static SkeletonGPU skeleton_gpu = game.renderer.upload_skeleton(SkeletonCPU{});
    return &skeleton_gpu.buffer;
}

void inspect(SkeletonGPU* skeleton, const m44& model, RenderScene* render_scene) {
    ImGui::Text("Skeleton");
    ImGui::Indent();
    bool any_changed = false;

    if (skeleton->widget_enabled.size() != skeleton->bones.size())
        skeleton->widget_enabled.resize(skeleton->bones.size());

    PoseWidgetSettings settings{*render_scene};
    if (ImGui::BeginTable("bones", 6, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Name/Id");
        ImGui::TableSetupColumn("Parent");
        ImGui::TableSetupColumn("Position");
        ImGui::TableSetupColumn("Rotation");
        ImGui::TableSetupColumn("Scale");
        ImGui::TableSetupColumn("Widget");
        ImGui::TableHeadersRow();
        for (id_ptr<Bone>& bone : skeleton->bones) {
            ImGui::PushID(bone.id);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", bone->name.empty() ? fmt_("{}", bone.id).c_str() : bone->name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", bone->parent.valid() ? (bone->parent->name.empty() ? fmt_("{}", bone->parent.id).c_str() : bone->parent->name.c_str()) : "");
            
            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            any_changed |= ImGui::DragFloat3("##Position", bone->start.position.value.data, 0.01f);               
            ImGui::TableSetColumnIndex(3);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            // euler e = math::to_euler(bone->start.rotation.value.data);
            if (ImGui::DragFloat4("##Rotation", bone->start.rotation.value.data, 0.01f)) {
                // bone->start.rotation.value = math::to_quat(e);
                any_changed = true;
            }
            ImGui::TableSetColumnIndex(4);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            any_changed |= ImGui::DragFloat3("##Scale", bone->start.scale.value.data, 0.01f);

            ImGui::TableSetColumnIndex(5);
            u32 bone_index = skeleton->bones.index(bone);
            u8& bone_widget_enabled = skeleton->widget_enabled[bone_index];
            ImGui::Checkbox("##Widget", (bool*) &bone_widget_enabled);

            if (bone_widget_enabled) {
                m44 mat = bone->parent.valid() ? model * bone->parent->transform() : model;
                any_changed |= pose_widget(bone.id, &bone->start.position.value, &bone->start.rotation.value, settings, &mat);
            }
            
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::InputText("Pose Name", &skeleton->pose_select);
    if (ImGui::BeginCombo("Pose", skeleton->pose_select.c_str(), ImGuiComboFlags_None)) {
        for (auto& [pose_name, pose_data] : skeleton->poses) {
            const bool is_selected = pose_name == skeleton->pose_select;
            if (ImGui::Selectable(pose_name.c_str(), is_selected))
                skeleton->pose_select = pose_name;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    if (ImGui::Button("Save")) {
        skeleton->save_pose(skeleton->pose_select);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        skeleton->load_pose(skeleton->pose_select);
        any_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load As Target")) {
        skeleton->load_pose(skeleton->pose_select, true, 1.0f);
        any_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Pose##Button")) {
        skeleton->stop_playing();
        any_changed = true;
    }
    
    ImGui::Checkbox("Render Lines", &skeleton->render_lines);

    if (any_changed || skeleton->mode == SkeletonGPU::Mode_Play)
        skeleton->update();
    
    if (skeleton->render_lines && render_scene != nullptr) {
        uset<u64> is_parent;
        for (u32 i = 0; i < skeleton->bones.size(); i++) {
            if (skeleton->bones[i]->parent.valid())
                is_parent.insert(skeleton->bones[i]->parent.id);
        }
        for (id_ptr<Bone> bone : skeleton->bones) {
            if (!is_parent.contains(bone.id)) {
                vector<FormattedVertex> vertices;
                id_ptr<Bone> current_node = bone;
                vertices.push_back({math::apply_transform(model * current_node->transform(), v3(0.0f)), palette::gray_5, 0.003f});
                while (current_node->parent.valid()) {
                    current_node = current_node->parent;
                    vertices.push_back({math::apply_transform(model * current_node->transform(), v3(0.0f)), palette::gray_5, 0.003f});
                }
                auto line_mesh = generate_formatted_line(render_scene->viewport.camera, std::move(vertices));
                if (line_mesh.file_path.empty())
                    continue;
                render_scene->quick_mesh(line_mesh, true, true);
            }

            m44 mat = model * bone->transform();
            v3 loc = math::apply_transform(mat, v3(0.0f));
            auto joint_mesh = generate_cube(loc, v3(0.01f), palette::gray_5);
            if (joint_mesh.file_path.empty())
                continue;
            render_scene->quick_mesh(joint_mesh, true, true);
        }
        
    }
    
}

}
