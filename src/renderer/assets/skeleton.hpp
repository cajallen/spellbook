#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "general/geometry.hpp"
#include "general/matrix.hpp"
#include "general/quaternion.hpp"
#include "general/id_ptr.hpp"


namespace spellbook {

struct RenderScene;

template<typename T>
struct KeyFrame {
    T value;
    float time;
};

struct KeySet {
    KeyFrame<v3> position;
    KeyFrame<quat> rotation;
    KeyFrame<v3> scale;
};

struct Bone {
    string name;
    id_ptr<Bone> parent;

    KeySet start;
    KeySet target;

    m44 inverse_bind_matrix;

    m44 local_transform;
    float time;
    math::EaseMode ease_mode = math::EaseMode_Quad;
    
    float length = 0.1f;

    m44 transform() const;
    m44 final_transform() const;
    void update(float new_time = -1.0f);
    m44 update_position();
    m44 update_rotation();
    m44 update_scaling();
};

struct SkeletonCPU {
    vector<id_ptr<Bone>> bones;
    umap<string, vector<KeySet>> poses;
    
    bool widget_enabled = false;
    vector<u8> widget_pose_enabled;

    void update();
    void save_pose(string name);
    void load_pose(string name, bool as_target = false, float offset = 1.0f);
};

struct SkeletonGPU {
    id_ptr<SkeletonCPU> skeleton_cpu;

    vuk::Unique<vuk::Buffer> buffer = vuk::Unique<vuk::Buffer>();
    
    static vuk::Unique<vuk::Buffer>* empty_buffer();
    void update();
};

SkeletonGPU upload_skeleton(id_ptr<SkeletonCPU> skeleton_cpu);

bool inspect(SkeletonCPU* skeleton, const m44& model, RenderScene* render_scene);

JSON_IMPL_TEMPLATE(template<typename T>, KeyFrame<T>, value, time);
JSON_IMPL(KeySet, position, rotation, scale);
JSON_IMPL(Bone, name, parent, start, inverse_bind_matrix, length);

inline SkeletonCPU from_jv_impl(const json_value& jv, SkeletonCPU* _) {
    json j = from_jv<json>(jv);
    SkeletonCPU value;
    if (j.contains("bones"))
        value.bones = from_jv<decltype(value.bones)>(*j.at("bones"));
    return value;
}
inline json_value to_jv(const SkeletonCPU& value) {
    auto j = json();
    
    vector<json_value> _list = {};
    for (id_ptr<Bone> e : value.bones)
        _list.push_back(to_jv_full(e));

    j["bones"] = make_shared<json_value>(_list);
    
    return to_jv(j);
}

}
