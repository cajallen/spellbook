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
#include "general/math/matrix_math.hpp"
#include "renderer/render_scene.hpp"
#include "editor/console.hpp"
#include "editor/pose_widget.hpp"
#include "editor/skeleton_widget.hpp"
#include "game/input.hpp"
#include "game/game.hpp"

namespace spellbook {

static v2 constr(v2 x, v2 y, float length) {
    return x + math::normalize(y - x) * length;
}

void apply_constraints(IKTarget ik) {
    vector<Bone*> bones;
    vector<float> lengths;
    vector<v3> points;
    bones.resize(ik.length);
    lengths.resize(ik.length);
    points.resize(ik.length + 1);
    {
        Bone* current = ik.source;
        points.back() = ik.target;
        ik.length--;
        while (ik.length >= 0) {
            points[ik.length] = current->calculate_position();
            bones[ik.length] = current;
            lengths[ik.length] = current->length;

            current = current->parent;
            ik.length--;
        }
    }

    apply_constraints(points, lengths);
    
    for (int32 i = 0; i < bones.size(); i++) {
        bool use_z_up = i == 0 || math::dot(math::normalize(points[i] - points[i-1]), math::normalize(points[i+1] - points[i])) > 0.99f;
        bones[i]->ik_transform = math::look_ik(points[i], points[i+1] - points[i], !use_z_up ? math::normalize(points[i] - points[i-1]) : v3(0.0f, 0.0f, 1.0f));
        bones[i]->ik_set_this_frame = true;
    }
}

void apply_constraints(vector<v3>& points, const vector<float>& lengths) {
    vector<v2> points_2d;
    v2 leg_vec = math::normalize((points.back() - points.front()).xy);
    for (const v3& point : points) {
        points_2d.push_back(v2(math::dot(point.xy - points.front().xy, leg_vec), point.z));
    }
    
    constexpr int iters = 100;
    vector<v2> out_points;
    vector<v2> in_points;
    out_points.resize(points.size());
    out_points.front() = points_2d.front();
    in_points.resize(points.size());
    
    for (uint32 iter_idx = 0; iter_idx < iters; iter_idx++) {
        for (uint32 i = 1; i < points.size(); i++) {
            v2 p1 = out_points[i-1];
            // use end if we don't have a previous iter, or if it's the last point
            v2 p2 = iter_idx == 0 || i + 1 == points_2d.size() ? points_2d[i] : in_points[i];

            out_points[i] = constr(p1,p2, lengths[i-1]);
        }
        if (iter_idx + 1 == iters)
            break;

        for (uint32 i = points.size() - 2; i > 0; i--) {
            // If it's the first step, start from the ending point, otherwise use previous step's point
            v2 p1 = i == points_2d.size() - 2 ? points_2d[i+1] : in_points[i+1];
            v2 p2 = out_points[i];

            in_points[i] = constr(p1, p2, lengths[i]);
        }
    }
    for (uint32 i = 1; i < points.size(); i++) {
        points[i] = v3(points.front().xy, 0.0f) + v3(leg_vec, 0.0f) * out_points[i].x + v3(0.0f, 0.0f, 1.0f) * out_points[i].y;
    }
}

SkeletonCPU instance_prefab(SkeletonPrefab& prefab) {
    umap<uint64, Bone*> bones;
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
        uint64 this_id = bone_prefab.id;
        uint64 parent_id = bone_prefab->parent.id;
        bones[this_id]->parent = bones.contains(parent_id) ? bones[parent_id] : nullptr;
    }
    return skeleton_cpu;
}

SkeletonGPU upload_skeleton(const SkeletonCPU& skeleton_cpu) {
    SkeletonGPU skeleton_gpu;
    vuk::Allocator& alloc = *game.renderer.global_allocator;
    uint32 alloc_size = sizeof(uint32) * 4 + sizeof(m44GPU) * skeleton_cpu.bones.size();

    skeleton_gpu.buffer = *vuk::allocate_buffer(alloc, {vuk::MemoryUsage::eCPUtoGPU, alloc_size, 1});
    return skeleton_gpu;
}

void SkeletonCPU::store_pose(const string& pose_name) {
    Pose* existing_pose = nullptr;
    for (auto& entry : prefab->pose_catalog)
        if (entry.name == pose_name)
            existing_pose = &entry;
    
    if (existing_pose)
        prefab->pose_catalog.remove_index(prefab->pose_catalog.index(*existing_pose));
    
    auto& entry = prefab->pose_catalog.push_back(Pose{pose_name});
    auto bone_map = &entry.bones;
    
    for (std::unique_ptr<Bone>& bone : bones) {
        (*bone_map)[bone->name] = bone->start;
    }
}

Bone* SkeletonCPU::find_bone(const string& name) {
    for (auto& bone : bones) {
        if (bone->name == name) {
            return &*bone;
        }
    }
    return nullptr;
}


void SkeletonCPU::load_pose(Pose& pose, float offset) {
    current_pose = "pose";
    for (std::unique_ptr<Bone>& bone : bones) {
        bone->ease_mode = math::EaseMode_Linear;
        if (offset <= 0.0f) {
            bone->start = pose.bones[bone->name];
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
        
        float used_offset = offset > 0.0f ? offset : 0.0f;
        bone->target = pose.bones[bone->name];
        bone->target.position.time = time + used_offset;
        bone->target.rotation.time = time + used_offset;
        bone->target.scale.time = time + used_offset;
    }
}


void SkeletonCPU::load_frame(AnimationFrame& entry, float offset) {
    current_pose = entry.pose->name;
    for (std::unique_ptr<Bone>& bone : bones) {
        bone->ease_mode = entry.ease_mode;
        if (offset <= 0.0f) {
            bone->start = entry.pose->bones[bone->name];
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
        bone->target = entry.pose->bones[bone->name];
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

m44 Bone::final_ik_transform() const {
    return ik_transform * inverse_bind_matrix;
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

v3 Bone::calculate_position() {
    v4 hpos = transform() * v4(0.0f, 0.0f, 0.0f, 1.0f);
    return hpos.xyz / hpos.w;
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
    vector<uint8> bones_data( sizeof(uint32) * 4 + sizeof(m44GPU) * skeleton.bones.size());
    bones_data.append_data(skeleton.bones.size());
    struct { uint32 a,b,c; } padding;
    bones_data.append_data(padding);
    for (const std::unique_ptr<Bone>& bone : skeleton.bones) {
        bones_data.append_data(m44GPU(bone->ik_set_this_frame ? bone->final_ik_transform() : bone->final_transform()));
        bone->ik_set_this_frame = false;
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
            int load_pose = INT_MAX;
            changed |= inspect(&prefab->animations[type], &load_pose);
            if (load_pose < prefab->animations[type].size()) {
                skeleton_cpu->load_frame(prefab->animations[type][load_pose], 0.0f);
            } else {
                assert_else(load_pose == INT_MAX);
            }
            ImGui::TreePop();
        }
    }

    struct PoseSetTakeState {
        AnimationState to_type;
        vector<uint8> selected;
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
        auto& animation = prefab->animations[take_state.to_type];
        for (int i = 0; i < prefab->pose_catalog.size(); i++) {
            if (take_state.selected[i]) {
                animation.push_back({&prefab->pose_catalog[i], 1.0f});
            }
        }
    }
    if (take_state.selected.size() != prefab->pose_catalog.size())
        take_state.selected.resize(prefab->pose_catalog.size());
    for (int i = 0; i < prefab->pose_catalog.size(); i++) {
        ImGui::PushID(i);
        if (ImGui::Button(skeleton_cpu->current_pose == prefab->pose_catalog[i].name ? ICON_FA_REFRESH : ICON_FA_CAMERA))
            skeleton_cpu->load_pose(prefab->pose_catalog[i], 0.0f);
        ImGui::SameLine();
        ImGui::InputText("Name", &prefab->pose_catalog[i].name);
        ImGui::SameLine();
        ImGui::Dummy(ImVec2{ImGui::GetContentRegionAvail().x - 60.f, 0.f});
        ImGui::SameLine();
        ImGui::Checkbox("##Selected", (bool*) &take_state.selected[i]);
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TIMES, {25.f, 0.f})) {
            prefab->pose_catalog.remove_index(i, false);
            i--;
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    
    return changed;
}

bool inspect(vector<AnimationFrame>* animation, int* load_pose) {
    return ImGui::OrderedVector(*animation,
        [](AnimationFrame& frame) {
            float width = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemWidth(width * 0.30f);
            ImGui::InputText("##Name", &frame.pose->name, ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(width * 0.30f);
            ImGui::DragFloat("##TimeTo", &frame.time_to, 0.01f, 0.0f, 10.0f, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(width * 0.30f);
            ImGui::EnumCombo("##EaseMode", &frame.ease_mode);
            return false;
        },
        std::function<void(vector<AnimationFrame>&,bool)>{},
        false);
}

AnimationFrame from_jv_impl(const json_value& jv, vector<Pose>& pose_catalog, AnimationFrame* _) {
    json j = from_jv<json>(jv);
    AnimationFrame value;
    FROM_JSON_ELE(time_to);
    FROM_JSON_ELE(ease_mode);

    string pose_name;
    if (j.contains("pose_name"))
        pose_name = from_jv<string>(*j.at("pose_name"));
    
    for (Pose& pose : pose_catalog)
        if (pose.name == pose_name)
            value.pose = &pose;
    
    return value;
}
json_value to_jv(const AnimationFrame& value) {
    auto j = json();
    TO_JSON_ELE(time_to);
    TO_JSON_ELE(ease_mode);
    j["pose_name"] = make_shared<json_value>(to_jv(value.pose->name));
    
    return to_jv(j);
}

template <>
bool     save_asset(const SkeletonPrefab& value) {
    json j;
    j["dependencies"] = make_shared<json_value>(to_jv(value.dependencies));
    vector<json_value> json_bones;
    for (id_ptr<BonePrefab> bone : value.bones) {
        json_bones.push_back(to_jv_full(bone));
    }
    j["bones"] = make_shared<json_value>(to_jv(json_bones));
    j["pose_catalog"] = make_shared<json_value>(to_jv(value.pose_catalog));
    j["animations"] = make_shared<json_value>(to_jv(value.animations));
    
    string ext = value.file_path.extension();
    assert_else(ext == extension(FileType_Skeleton))
        return false;
    
    file_dump(j, value.file_path.abs_string());
    return true;
}

template <>
SkeletonPrefab& load_asset(const FilePath& input_path, bool assert_exists, bool clear_cache) {
    fs::path absolute_path = input_path.abs_string();
    if (clear_cache && cpu_asset_cache<SkeletonPrefab>().contains(input_path))
        cpu_asset_cache<SkeletonPrefab>().erase(input_path);
    if (cpu_asset_cache<SkeletonPrefab>().contains(input_path))
        return *cpu_asset_cache<SkeletonPrefab>()[input_path];

    SkeletonPrefab& value = *cpu_asset_cache<SkeletonPrefab>().emplace(input_path, std::make_unique<SkeletonPrefab>()).first->second;

    bool exists = fs::exists(absolute_path);
    bool corrext = input_path.extension() == extension(from_typeinfo(typeid(SkeletonPrefab)));
    if (assert_exists) {
        assert_else(exists && corrext)
            return value;
    } else {
        check_else(exists && corrext)
            return value;
    }
    
    json j = parse_file(absolute_path.string());
    value.file_path = input_path;
    value.dependencies = game.asset_system.load_dependencies(j);

    if (j.contains("bones")) {
        for (const json_value& jv : j["bones"]->get_list()) {
            id_ptr<BonePrefab> bone = from_jv_impl(jv, (id_ptr<BonePrefab>*) 0);
            value.bones.push_back(bone);
        }
    }
    if (j.contains("pose_catalog"))
        value.pose_catalog = from_jv<decltype(value.pose_catalog)>(*j.at("pose_catalog"));

    if (j.contains("animations")) {
        int i = 0;
        for (const json_value& jv_anim : j["animations"]->get_list()) {
            for (const json_value& jv_frame : jv_anim.get_list()) {
                AnimationFrame frame = from_jv_impl(jv_frame, value.pose_catalog, (AnimationFrame*) 0);
                value.animations[i].push_back(frame);
            }
            i++;
        }
    }
    
    return value;
}



}
