#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "game/scene.hpp"
#include "general/geometry.hpp"
#include "general/matrix.hpp"
#include "general/quaternion.hpp"
#include "general/id_ptr.hpp"


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

struct PoseSet {
    enum Type {
        Type_Idle,
        Type_Flail,
        Type_Walking,
        Type_Attacking,
        Type_Attacked, // At point of attacking, pick if we're attacking again or not, pick attacked animation according
        TypeCount
    };

    struct Entry {
        string name;
        float time_to = 0.0f;
        umap<string, KeySet> pose;
    };
    
    vector<Entry> entries;

    Entry* get_entry(const string& name) {
        for (auto& entry : entries)
            if (entry.name == name)
                return &entry;
        return nullptr;
    }
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

    vector<id_ptr<BonePrefab>> bones;
    array<PoseSet, PoseSet::TypeCount> poses;

    PoseSet pose_backfill;
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
    
    m44 transform() const;
    m44 final_transform() const;
    void update(float new_time = -1.0f);
    m44 update_position();
    m44 update_rotation();
    m44 update_scaling();
};

struct SkeletonCPU {
    SkeletonPrefab* prefab;
    vector<std::unique_ptr<Bone>> bones;
    string current_pose;

    float time = 0.0f;

    void update();
    void save_pose(PoseSet::Type pose_set, string pose_name, float timing, int pose_index = -1);
    void load_pose(PoseSet::Entry& pose_entry, float offset = -1.0f);
    void store_pose(const string& pose_name);
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
JSON_IMPL(BonePrefab, name, parent, position, inverse_bind_matrix);
JSON_IMPL(PoseSet::Entry, name, time_to, pose);
JSON_IMPL(PoseSet, entries);

template <>
bool     save_asset(const SkeletonPrefab& asset_file);
template <>
SkeletonPrefab& load_asset(const string& input_path, bool assert_exist);

bool inspect(SkeletonCPU* skeleton_cpu);
bool inspect(PoseSet* pose_set, int* load_pose = nullptr);

}
