#include "skeleton.hpp"

#include <tracy/Tracy.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/AllocatorHelpers.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "editor/console.hpp"
#include "editor/pose_widget.hpp"
#include "editor/widget_system.hpp"
#include "extension/fmt.hpp"
#include "general/matrix_math.hpp"
#include "general/logger.hpp"
#include "game/input.hpp"
#include "game/game.hpp"
#include "renderer/draw_functions.hpp"
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

void SkeletonCPU::update() {
    for (auto bone_ptr : bones) {
        bone_ptr->update(true ? -1.0f : Input::time);
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

void skeleton_widget(SkeletonCPU* skeleton, const m44& model, RenderScene* render_scene);

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

    if (any_changed) // || playing
        skeleton->update();
    
    if (skeleton->widget_enabled && render_scene != nullptr) {
        skeleton_widget(skeleton, model, render_scene);
    }
    return any_changed;
}

struct Mouse3DInfo {
    m44 model;
    m44 mvp;
    v2 uv_position;
    ray3 os_ray;
    ray3 os_center_ray;

    v2 viewport_size;
};

struct LineProjectInfo {
    float distance = 0.0f;
    float axis_value = 0.0f;
    float visual_axis_value = 0.0f;
    v3 position = v3(0.0f);
};

LineProjectInfo skeleton_mouse_to_3d_line(const Mouse3DInfo& mouse, float radius, int axis) {
    LineProjectInfo return_info;

    auto axis_ray = ray3(v3(0.0f), v3(0.0f));
    axis_ray.dir[axis] = 1.0f;
    
    return_info.axis_value = math::line_intersection_3d(axis_ray, mouse.os_ray);
    return_info.visual_axis_value = math::clamp(return_info.axis_value, math::min(0.0f, radius), math::max(0.0f, radius));
    
    v3 axis_projected = v3(0.0f);
    axis_projected[axis] = return_info.visual_axis_value;
    v4 h_screen_axis = mouse.mvp * v4(axis_projected, 1.0f);
    v2 uv_axis_position = math::to_unsigned_range(h_screen_axis.xy / h_screen_axis.w);
    return_info.distance = math::length((uv_axis_position - mouse.uv_position) * mouse.viewport_size);
    return_info.position[axis] = return_info.visual_axis_value;
    
    return return_info;
}

void skeleton_widget(SkeletonCPU* skeleton, const m44& model, RenderScene* render_scene) {
    static bool initialized = false;
    static string joint_name = "";
    if (!initialized) {
        auto joint_mesh = generate_cube(v3(0.0f), v3(0.01f), palette::gray_5);
        joint_name = upload_mesh(joint_mesh, false);
        initialized = true;
    }

    ImGui::Text("pressed_widget: %d", WidgetSystem::pressed_id);
    
    for (id_ptr<Bone>& bone : skeleton->bones) {
        ZoneScoped;
        m44 bone_transform = model * bone->transform();
        auto& r = render_scene->quick_mesh(joint_name, true, true);
        r.transform = (m44GPU) bone_transform;

        auto line = generate_formatted_line(render_scene->viewport.camera, {
            {math::apply_transform(bone_transform, v3(0.0f)), palette::gray_5, 0.003f},
            {math::apply_transform(bone_transform, v3(0.0f, bone->length, 0.0f)), palette::gray_5, 0.003f}
        });
        render_scene->quick_mesh(line, true, true);

        Mouse3DInfo mouse;
        mouse.model = bone_transform;
        mouse.mvp = render_scene->viewport.camera->vp * mouse.model;
        mouse.uv_position = render_scene->viewport.mouse_uv();
        mouse.os_ray = math::transformed_ray(mouse.mvp, mouse.uv_position);
        v4 h_screen_position = mouse.mvp * v4(v3(0.0f), 1.0f);
        v2 uv_screen_position = math::to_unsigned_range(h_screen_position.xy / h_screen_position.w);
        mouse.os_center_ray = math::transformed_ray(mouse.mvp, uv_screen_position);
        mouse.viewport_size = v2(render_scene->viewport.size);

        auto line_project_info = skeleton_mouse_to_3d_line(mouse, bone->length, 1);

        auto& depth = WidgetSystem::depths[bone.id];
        if (line_project_info.distance < 5.0f) {
            constexpr float bias = 0.02f;
            depth = math::min(depth, math::length(line_project_info.position - render_scene->viewport.camera->position) + bias);
        } else {
            depth = math::min(depth, FLT_MAX);
        }
        
        if (Input::mouse_click[0]) {
            if (depth < FLT_MAX) {
                console({"test"});
            } else {
                skeleton->widget_pose_enabled[skeleton->bones.index(bone)] = false;
            }
        }

        auto pressed_id = WidgetSystem::pressed_id;
        if (pressed_id != 0)
            console({"test"});
        if (pressed_id == bone.id)
            skeleton->widget_pose_enabled[skeleton->bones.index(bone)] = true;
    }
}


}
