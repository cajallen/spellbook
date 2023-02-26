#include "skeleton.hpp"

#include <tracy/Tracy.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/AllocatorHelpers.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <magic_enum.hpp>

#include "extension/fmt.hpp"
#include "extension/icons/font_awesome4.h"
#include "extension/imgui_extra.hpp"
#include "general/matrix_math.hpp"
#include "renderer/render_scene.hpp"
#include "editor/console.hpp"
#include "editor/pose_widget.hpp"
#include "editor/skeleton_widget.hpp"
#include "game/input.hpp"
#include "game/game.hpp"

namespace spellbook {

SkeletonCPU instance_prefab(SkeletonPrefab& prefab) {
    umap<u64, Bone*> bones;
    SkeletonCPU skeleton_cpu;
    skeleton_cpu.prefab = &prefab;
    skeleton_cpu.current_pose = "default";
    for (const id_ptr<BonePrefab>& bone_prefab : prefab.bones) {
        auto temp_bone = std::make_unique<Bone>();
        Bone* bone = &*skeleton_cpu.bones.emplace_back(std::move(temp_bone));
        bone->name = bone_prefab->name;
        bone->inverse_bind_matrix = bone_prefab->inverse_bind_matrix;
        bone->start = bone_prefab->position;
        bone->length = bone_prefab->length;
        bones[bone_prefab.id] = bone;
    }

    for (const id_ptr<BonePrefab>& bone_prefab : prefab.bones) {
        u64 this_id = bone_prefab.id;
        u64 parent_id = bone_prefab->parent.id;
        bones[this_id]->parent = bones.contains(parent_id) ? bones[parent_id] : nullptr;
    }
    return skeleton_cpu;
}

SkeletonGPU upload_skeleton(const SkeletonCPU& skeleton_cpu) {
    SkeletonGPU skeleton_gpu;
    vuk::Allocator& alloc = *game.renderer.global_allocator;
    u32 alloc_size = sizeof(u32) * 4 + sizeof(m44GPU) * skeleton_cpu.bones.size();

    skeleton_gpu.buffer = *vuk::allocate_buffer(alloc, {vuk::MemoryUsage::eCPUtoGPU, alloc_size, 1});
    return skeleton_gpu;
}

void SkeletonCPU::save_pose(AnimationState set_type, string pose_name, float timing, int pose_index) {
    PoseSet& pose_set = prefab->poses[set_type];

    PoseSet::Entry* existing_entry = pose_set.get_entry(pose_name);
    if (existing_entry) {
        pose_set.entries.remove_index(pose_set.entries.index(*existing_entry));
    }

    umap<string, KeySet>* bone_map;
    if (pose_index == -1) {
        auto& entry = pose_set.entries.emplace_back(pose_name, timing);
        bone_map = &entry.pose;
    } else {
        auto& entry = pose_set.entries.emplace(pose_index, pose_name, timing);
        bone_map = &entry.pose;
    }
    for (std::unique_ptr<Bone>& bone : bones) {
        (*bone_map)[bone->name] = bone->start;
    }
}

void SkeletonCPU::store_pose(const string& pose_name) {
    PoseSet& pose_set = prefab->pose_backfill;

    PoseSet::Entry* existing_entry = pose_set.get_entry(pose_name);
    if (existing_entry) {
        pose_set.entries.remove_index(pose_set.entries.index(*existing_entry));
    }
    
    auto& entry = pose_set.entries.emplace_back(pose_name, 0.0f);
    auto bone_map = &entry.pose;
    
    for (std::unique_ptr<Bone>& bone : bones) {
        (*bone_map)[bone->name] = bone->start;
    }
}

void SkeletonCPU::load_pose(PoseSet::Entry& entry, float offset) {
    current_pose = entry.name;
    for (std::unique_ptr<Bone>& bone : bones) {
        bone->ease_mode = entry.ease_mode;
        if (offset == 0.0f) {
            bone->start = entry.pose[bone->name];
            bone->target.position.time = -1.0f;
            bone->target.rotation.time = -1.0f;
            bone->target.scale.time = -1.0f;
            continue;
        }
        
        if (bone->target.position.time != -1.0f)
            bone->start.position.value = bone->target.position.value;
        if (bone->target.rotation.time != -1.0f)
            bone->start.rotation.value = bone->target.rotation.value;
        if (bone->target.scale.time != -1.0f)
            bone->start.scale.value = bone->target.scale.value;
        bone->start.position.time = time;
        bone->start.rotation.time = time;
        bone->start.scale.time = time;
        
        float used_offset = offset > 0.0f ? offset : entry.time_to;
        bone->target = entry.pose[bone->name];
        bone->target.position.time = time + used_offset;
        bone->target.rotation.time = time + used_offset;
        bone->target.scale.time = time + used_offset;
    }
}


m44 Bone::transform() const {
    if (parent != nullptr)
        return parent->transform() * local_transform;
    return local_transform;
}

m44 Bone::final_transform() const {
    if (parent != nullptr)
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
    if (time == -1.0f || target.rotation.time == -1.0f)
        return math::rotation(start.rotation.value);

    float t = math::from_range(time, range(start.rotation.time, target.rotation.time));
    t = math::clamp(t, range(0.0f, 1.0f));
    //t = math::abs(1.0f - math::mod(t, 2.0f));
    quat interpolated = math::slerp(start.rotation.value, target.rotation.value, math::ease(t, ease_mode));
    return math::rotation(interpolated);
}

m44 Bone::update_scaling() {
    if (time == -1.0f || target.scale.time == -1.0f)
        return math::scale(start.scale.value);

    float t = math::from_range(time, range(start.scale.time, target.scale.time));
    t = math::clamp(t, range(0.0f, 1.0f));
    //t = math::abs(1.0f - math::mod(t, 2.0f));
    v3 interpolated = math::mix(start.scale.value, target.scale.value, math::ease(t, ease_mode));
    return math::scale(interpolated);
}

void SkeletonCPU::update() {
    for (auto& bone : bones) {
        bone->update(time);
    }
}

void SkeletonGPU::update(const SkeletonCPU& skeleton) {
    vector<u8> bones_data( sizeof(u32) * 4 + sizeof(m44GPU) * skeleton.bones.size());
    bones_data.append_data(skeleton.bones.size());
    struct { u32 a,b,c; } padding;
    bones_data.append_data(padding);
    for (const std::unique_ptr<Bone>& bone : skeleton.bones) {
        bones_data.append_data((m44GPU) bone->final_transform());
    }
    memcpy(buffer->mapped_ptr, bones_data.data(), bones_data.size());
}

vuk::Unique<vuk::Buffer>* SkeletonGPU::empty_buffer() {
    static SkeletonCPU empty_skeleton = {};
    static SkeletonGPU skeleton_gpu = upload_skeleton(empty_skeleton);
    return &skeleton_gpu.buffer;
}


bool inspect(SkeletonCPU* skeleton_cpu) {
    SkeletonPrefab* prefab = skeleton_cpu->prefab;
    
    bool changed = false;
    ImGui::Text("Poses");
    for (int i = 0; i < magic_enum::enum_count<AnimationState>() - 1; i++) {
        auto type = AnimationState(i);

        string enum_name = string(magic_enum::enum_name(AnimationState(i)));
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(enum_name.c_str())) {
            int load_pose;
            changed |= inspect(&prefab->poses[type], &load_pose);
            if (load_pose > 0) {
                skeleton_cpu->load_pose(prefab->poses[type].entries[load_pose], 0.0f);
            }
            ImGui::TreePop();
        }
    }

    struct PoseSetTakeState {
        AnimationState to_type;
        vector<u8> selected;
    };
    static umap<SkeletonPrefab*, PoseSetTakeState> take_states;

    if (!take_states.contains(prefab))
        take_states[prefab] = {};
    auto& take_state = take_states[prefab];
    
    ImGui::Separator();
    ImGui::Text("Extras");
    ImGui::EnumCombo("To Type", &take_state.to_type);
    ImGui::SameLine();
    if (ImGui::Button("Migrate")) {
        auto& pose_set = prefab->poses[take_state.to_type];
        for (int i = 0; i < prefab->pose_backfill.entries.size(); i++) {
            if (take_state.selected[i]) {
                pose_set.entries.push_back(prefab->pose_backfill.entries[i]);
            }
        }
    }
    if (take_state.selected.size() != prefab->pose_backfill.entries.size())
        take_state.selected.resize(prefab->pose_backfill.entries.size());
    for (int i = 0; i < prefab->pose_backfill.entries.size(); i++) {
        ImGui::PushID(i);
        if (ImGui::Button(skeleton_cpu->current_pose == prefab->pose_backfill.entries[i].name ? ICON_FA_REFRESH : ICON_FA_CAMERA))
            skeleton_cpu->load_pose(prefab->pose_backfill.entries[i], 0.0f);
        ImGui::SameLine();
        ImGui::InputText("Name", &prefab->pose_backfill.entries[i].name);
        ImGui::SameLine();
        ImGui::Dummy(ImVec2{ImGui::GetContentRegionAvail().x - 60.f, 0.f});
        ImGui::SameLine();
        ImGui::Checkbox("##Selected", (bool*) &take_state.selected[i]);
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TIMES, {25.f, 0.f})) {
            prefab->pose_backfill.entries.remove_index(i, false);
            i--;
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    
    return changed;
}

bool inspect(PoseSet* pose_set, int* load_pose) {
    ImGui::PushID(pose_set);
    bool changed = false;
    for (int i = 0; i < pose_set->entries.size(); i++) {
        ImGui::PushID(i);
        if (load_pose) {
            *load_pose = -1;
            if (ImGui::Button(ICON_FA_CAMERA))
                *load_pose = i;
            ImGui::SameLine();
        }
        float width = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(width * 0.33f - 50.0f);
        ImGui::InputText("##Name", &pose_set->entries[i].name);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(width * 0.33f - 50.0f);
        changed |= ImGui::DragFloat("Time To", &pose_set->entries[i].time_to, 0.01f);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(width * 0.33f - 50.0f);
        changed |= ImGui::EnumCombo("##EaseMode", &pose_set->entries[i].ease_mode);
        ImGui::SameLine();
        if (i > 0) {
            if (ImGui::Button(ICON_FA_ARROW_UP, {25.f, 0.f})) {
                std::swap(pose_set->entries[i-1], pose_set->entries[i]);
                changed = true;
                ImGui::PopID();
                break;
            }
        } else {
            ImGui::Dummy(ImVec2{25.f, 0.f});
        }
        ImGui::SameLine();
        if (i + 1 < pose_set->entries.size()) {
            if (ImGui::Button(ICON_FA_ARROW_DOWN, {25.f, 0.f})) {
                std::swap(pose_set->entries[i], pose_set->entries[i+1]);
                changed = true;
                ImGui::PopID();
                break;
            }
        } else {
            ImGui::Dummy(ImVec2{25.f, 0.f}); 
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TIMES, {25.f, 0.f})) {
            pose_set->entries.remove_index(i, false);
            changed = true;
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
        ImGui::Separator();
    }
    ImGui::PopID();
    return changed;
}

template <>
bool     save_asset(const SkeletonPrefab& value) {
    json j;
    vector<json_value> json_bones;
    for (id_ptr<BonePrefab> bone : value.bones) {
        json_bones.push_back(to_jv_full(bone));
    }
    j["bones"] = make_shared<json_value>(to_jv(json_bones));
    TO_JSON_ELE(poses);
    TO_JSON_ELE(pose_backfill);

    string ext = std::filesystem::path(value.file_path).extension().string();
    assert_else(ext == extension(FileType_Skeleton))
        return false;
    
    file_dump(j, to_resource_path(value.file_path).string());
    return true;
}

template <>
SkeletonPrefab& load_asset(const string& input_path, bool assert_exists) {
    fs::path absolute_path = to_resource_path(input_path);
    if (asset_cache<SkeletonPrefab>().contains(absolute_path.string()))
        return *asset_cache<SkeletonPrefab>()[absolute_path.string()];

    SkeletonPrefab& value = *asset_cache<SkeletonPrefab>().emplace(absolute_path.string(), std::make_unique<SkeletonPrefab>()).first->second;

    string ext = absolute_path.extension().string();
    bool exists = fs::exists(absolute_path.string());
    bool corrext = ext == extension(from_typeinfo(typeid(SkeletonPrefab)));
    if (assert_exists) {
        assert_else(exists && corrext)
            return value;
    } else {
        check_else(exists && corrext)
            return value;
    }
    
    json      j = parse_file(absolute_path.string());
    value.file_path = input_path;

    if (j.contains("bones")) {
        for (const json_value& jv : j["bones"]->get_list()) {
            id_ptr<BonePrefab> bone = from_jv_impl(jv, (id_ptr<BonePrefab>*) 0);
            value.bones.push_back(bone);
        }
    }
    FROM_JSON_ELE(poses);
    FROM_JSON_ELE(pose_backfill);
    
    return value;
}



}
