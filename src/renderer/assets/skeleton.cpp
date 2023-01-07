#include "skeleton.hpp"

#include <tracy/Tracy.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/AllocatorHelpers.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "editor/console.hpp"
#include "editor/pose_widget.hpp"
#include "editor/skeleton_widget.hpp"
#include "extension/fmt.hpp"
#include "general/matrix_math.hpp"
#include "game/input.hpp"
#include "game/game.hpp"
#include "renderer/render_scene.hpp"

namespace spellbook {

SkeletonGPU upload_skeleton(id_ptr<SkeletonCPU> skeleton_cpu) {
    SkeletonGPU skeleton_gpu;
    vuk::Allocator& alloc = *game.renderer.global_allocator;
    u32 alloc_size = sizeof(u32) * 4 + sizeof(m44GPU) * skeleton_cpu->bones.size();

    skeleton_gpu.skeleton_cpu = skeleton_cpu;
    skeleton_gpu.buffer = *vuk::allocate_buffer(alloc, {vuk::MemoryUsage::eCPUtoGPU, alloc_size, 1});
    skeleton_gpu.update();
    return skeleton_gpu;
}

void SkeletonCPU::save_pose(string name) {
    if (!poses.contains(name)) {
        poses[name] = vector<KeySet>();
        poses[name].resize(bones.size());
    }
    
    for (u32 i = 0; i < bones.size(); i++) {
        poses[name][i] = bones[i]->start;
    }
}

void SkeletonCPU::load_pose(string name, bool as_target, float offset) {
    console({.str=name});
    current_pose = name;
    for (u32 i = 0; i < bones.size(); i++) {
        if (!as_target) {
            bones[i]->start = poses[name][i];
            bones[i]->target.position.time = -1.0f;
            bones[i]->target.rotation.time = -1.0f;
            bones[i]->target.scale.time = -1.0f;
        }
        else {
            if (bones[i]->target.position.time != -1.0f)
                bones[i]->start.position.value = bones[i]->target.position.value;
            if (bones[i]->target.rotation.time != -1.0f)
                bones[i]->start.rotation.value = bones[i]->target.rotation.value;
            if (bones[i]->target.scale.time != -1.0f)
                bones[i]->start.scale.value = bones[i]->target.scale.value;
            bones[i]->start.position.time = time;
            bones[i]->start.rotation.time = time;
            bones[i]->start.scale.time = time;
            
            bones[i]->target = poses[name][i];
            bones[i]->target.position.time = time + offset;
            bones[i]->target.rotation.time = time + offset;
            bones[i]->target.scale.time = time + offset;
        }
    }
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
    if (time == -1.0f || target.position.time == -1.0f)
        return math::translate(start.position.value);
    
    float t = math::from_range(time, range(start.position.time, target.position.time));
    t = math::clamp(t, range(0.0f, 1.0f));
    //t = math::abs(1.0f - math::mod(t, 2.0f));
    v3 interpolated = math::mix(start.position.value, target.position.value, math::ease(t, ease_mode));
    return math::translate(interpolated);
}

m44 Bone::update_rotation() {
    if (time == -1.0f || target.position.time == -1.0f)
        return math::rotation(start.rotation.value);

    float t = math::from_range(time, range(start.rotation.time, target.rotation.time));
    t = math::clamp(t, range(0.0f, 1.0f));
    //t = math::abs(1.0f - math::mod(t, 2.0f));
    quat interpolated = math::slerp(start.rotation.value, target.rotation.value, math::ease(t, ease_mode));
    return math::rotation(interpolated);
}

m44 Bone::update_scaling() {
    if (time == -1.0f || target.position.time == -1.0f)
        return math::scale(start.scale.value);

    float t = math::from_range(time, range(start.scale.time, target.scale.time));
    t = math::clamp(t, range(0.0f, 1.0f));
    //t = math::abs(1.0f - math::mod(t, 2.0f));
    v3 interpolated = math::mix(start.scale.value, target.scale.value, math::ease(t, ease_mode));
    return math::scale(interpolated);
}

void SkeletonCPU::update() {
    for (auto bone_ptr : bones) {
        bone_ptr->update(time);
    }
}

void SkeletonGPU::update() {
    skeleton_cpu->update();
    vector<u8> bones_data( sizeof(u32) * 4 + sizeof(m44GPU) * skeleton_cpu->bones.size());
    bones_data.append_data(skeleton_cpu->bones.size());
    struct { u32 a,b,c; } padding;
    bones_data.append_data(padding);
    for (id_ptr<Bone> bone : skeleton_cpu->bones) {
        bones_data.append_data((m44GPU) bone->final_transform());
    }
    memcpy(buffer->mapped_ptr, bones_data.data(), bones_data.size());
}

vuk::Unique<vuk::Buffer>* SkeletonGPU::empty_buffer() {
    static id_ptr<SkeletonCPU> empty_skeleton = id_ptr<SkeletonCPU>::emplace();
    static SkeletonGPU skeleton_gpu = upload_skeleton(empty_skeleton);
    return &skeleton_gpu.buffer;
}

bool inspect(SkeletonCPU* skeleton, const m44& model, RenderScene* render_scene) {
    ZoneScoped;
    ImGui::Text("Skeleton");
    ImGui::Indent();
    bool any_changed = false;

    if (skeleton->widget_pose_enabled.size() != skeleton->bones.size())
        skeleton->widget_pose_enabled.resize(skeleton->bones.size());

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
            euler e = math::to_euler(bone->start.rotation.value.data);
            if (ImGui::DragFloat4("##Rotation", bone->start.rotation.value.data, 0.01f)) {
                bone->start.rotation.value = math::to_quat(e);
                any_changed = true;
            }
            ImGui::TableSetColumnIndex(4);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            any_changed |= ImGui::DragFloat3("##Scale", bone->start.scale.value.data, 0.01f);

            ImGui::TableSetColumnIndex(5);
            u32 bone_index = skeleton->bones.index(bone);
            u8& bone_widget_enabled = skeleton->widget_pose_enabled[bone_index];
            ImGui::Checkbox("##Widget", (bool*) &bone_widget_enabled);

            if (bone_widget_enabled) {
                m44 mat = bone->parent.valid() ? model * bone->parent->transform() : model;
                any_changed |= pose_widget(bone.id, &bone->start.position.value, &bone->start.rotation.value, settings, &mat);
            }
            
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    static umap<SkeletonCPU*, string> pose_selects;
    if (!pose_selects.contains(skeleton))
        pose_selects[skeleton] = "";

    string& pose_select = pose_selects[skeleton];
    
    ImGui::InputText("Pose Name", &pose_select);
    if (ImGui::BeginCombo("Pose", pose_select.c_str(), ImGuiComboFlags_None)) {
        for (auto& [pose_name, pose_data] : skeleton->poses) {
            const bool is_selected = pose_name == pose_select;
            if (ImGui::Selectable(pose_name.c_str(), is_selected))
                pose_select = pose_name;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    if (ImGui::Button("Save")) {
        skeleton->save_pose(pose_select);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        skeleton->load_pose(pose_select);
        any_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load As Target")) {
        skeleton->load_pose(pose_select, true, 1.0f);
        any_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Pose##Button")) {
        //skeleton->stop_playing();
        any_changed = true;
    }
    
    ImGui::Checkbox("3D Widget", &skeleton->widget_enabled);

    if (any_changed || true) // || playing
        skeleton->update();
    
    if (skeleton->widget_enabled && render_scene != nullptr) {
        skeleton_widget(skeleton, model, render_scene);
    }
    return any_changed;
}


}
