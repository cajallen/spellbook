#include "skeleton.hpp"

#include <vuk/Buffer.hpp>

#include "lib/matrix_math.hpp"
#include "game/input.hpp"
#include "game/game.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"


namespace spellbook {

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
        time = 0.0f;
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
    
    v3 interpolated = math::mix(start.position.value, target.position.value, math::from_range(time, range(start.position.time, target.position.time)));
    return math::translate(interpolated);
}

m44 Bone::update_rotation() {
    if (time == -1.0f)
        return math::rotation(start.rotation.value);
    
    quat interpolated = math::slerp(start.rotation.value, target.rotation.value, math::from_range(time, range(start.rotation.time, target.rotation.time)));
    return math::rotation(interpolated);
}

m44 Bone::update_scaling() {
    if (time == -1.0f)
        return math::scale(start.scale.value);
    
    v3 interpolated = math::mix(start.scale.value, target.scale.value, math::from_range(time, range(start.scale.time, target.scale.time)));
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
                warn_else(false && "Skeleton update mode NYI");
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

void inspect(std::unique_ptr<SkeletonGPU>& skeleton, RenderScene* render_scene) {
    ImGui::Text("Skeleton");
    ImGui::Indent();
    bool any_changed = false;
    
    if (ImGui::BeginTable("bones", 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Name/Id");
        ImGui::TableSetupColumn("Parent");
        ImGui::TableSetupColumn("Position");
        ImGui::TableSetupColumn("Rotation");
        ImGui::TableSetupColumn("Scale");
        ImGui::TableHeadersRow();
        for (id_ptr<Bone> bone : skeleton->bones) {
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
            euler e = math::to_euler(bone->start.rotation.value.data);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::DragFloat3("##Rotation", e.data, 0.01f)) {
                bone->start.rotation.value = math::to_quat(e);
                any_changed = true;
            }
            ImGui::TableSetColumnIndex(4);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            any_changed |= ImGui::DragFloat3("##Scale", bone->start.scale.value.data, 0.01f);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::Checkbox("Render Lines", &skeleton->render_lines);
    
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
                v4 current_hpos = current_node->final_transform() * v4(0,0,0,1);
                vertices.push_back({current_hpos.xyz / current_hpos.w, palette::white, 0.03f});
                while (current_node->parent.valid()) {
                    current_node = current_node->parent;
                    current_hpos = current_node->final_transform() * v4(0,0,0,1);
                    vertices.push_back({current_hpos.xyz / current_hpos.w, palette::white, 0.03f});
                }
                auto line_mesh = generate_formatted_line(render_scene->viewport.camera, std::move(vertices));
                if (line_mesh.file_path.empty())
                    continue;
                render_scene->quick_mesh(line_mesh);
            }
        }
        
    }

    if (any_changed)
        skeleton->update();
}

}
