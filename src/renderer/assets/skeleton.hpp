#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "game/game_file.hpp"
#include "general/geometry.hpp"
#include "general/matrix.hpp"
#include "general/quaternion.hpp"
#include "general/id_ptr.hpp"

#include "renderer/assets/animation_state.hpp"

namespace spellbook {

struct RenderScene;

template<typename T>
struct KeyFrame {
    T value;
    float time = -1.0f;
};

struct KeySet {
    KeyFrame<v3> position;
    KeyFrame<quat> rotation;
    KeyFrame<v3> scale;
};

struct Pose {
    string name;
    umap<string, KeySet> bones;
};

struct AnimationFrame {
    Pose* pose = nullptr;
    float time_to = 0.0f;
    math::EaseMode ease_mode = math::EaseMode_Quad;
};

struct BonePrefab {
    string name;
    id_ptr<BonePrefab> parent;
    KeySet position; // not technically needed
    m44 inverse_bind_matrix;
    float length = 0.1f;
};

struct SkeletonPrefab {
    string file_path;
    vector<string> dependencies;

    vector<id_ptr<BonePrefab>> bones;
    array<vector<AnimationFrame>, AnimationStateCount> animations;

    vector<Pose> pose_catalog;
};

struct Bone {
    string name;
    Bone* parent = nullptr;

    m44 inverse_bind_matrix = {};
    float length = 0.1f;
    
    math::EaseMode ease_mode = math::EaseMode_Quad;
    KeySet start = {};
    KeySet target = {};

    float time = 0.0f;
    m44 local_transform = {};

    m44 ik_transform = {};
    bool ik_set_this_frame;
    
    m44 transform() const;
    m44 final_transform() const;
    m44 final_ik_transform() const;
    void update(float new_time = -1.0f);
    m44 update_position();
    m44 update_rotation();
    m44 update_scaling();

    v3 calculate_position();
};

struct IKTarget {
    Bone* source;
    v3 target;
    int length;
};

struct SkeletonCPU {
    SkeletonPrefab* prefab;
    vector<std::unique_ptr<Bone>> bones;
    string current_pose;

    vector<IKTarget> iks;

    float time = 0.0f;

    void update();
    void load_pose(Pose& pose, float offset = -1.0f);
    void load_frame(AnimationFrame& frame, float offset = -1.0f);
    void store_pose(const string& pose_name);

    Bone* find_bone(const string& name);
};

struct SkeletonGPU {
    vuk::Unique<vuk::Buffer> buffer = vuk::Unique<vuk::Buffer>();
    
    static vuk::Unique<vuk::Buffer>* empty_buffer();
    void update(const SkeletonCPU& skeleton);
};

SkeletonCPU instance_prefab(SkeletonPrefab& prefab);
SkeletonGPU upload_skeleton(const SkeletonCPU& skeleton_cpu);

JSON_IMPL_TEMPLATE(template<typename T>, KeyFrame<T>, value, time);
JSON_IMPL(KeySet, position, rotation, scale);
JSON_IMPL(BonePrefab, name, parent, position, inverse_bind_matrix, length);
JSON_IMPL(Pose, name, bones);


inline AnimationFrame from_jv_impl(const json_value& jv, vector<Pose>& pose_catalog, AnimationFrame* _);
inline json_value to_jv(const AnimationFrame& value);

template <>
bool     save_asset(const SkeletonPrefab& asset_file);
template <>
SkeletonPrefab& load_asset(const string& input_path, bool assert_exist, bool clear_cache);

bool inspect(SkeletonCPU* skeleton_cpu);
bool inspect(vector<AnimationFrame>* animation, int* load_pose);

void apply_constraints(vector<v3>& points, const vector<float>& lengths);
void apply_constraints(IKTarget ik);

}
