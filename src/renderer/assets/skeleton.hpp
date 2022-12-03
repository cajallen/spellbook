#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "lib/geometry.hpp"
#include "lib/matrix.hpp"
#include "lib/quaternion.hpp"
#include "lib/id_ptr.hpp"

namespace spellbook {

struct KeyPosition {
    v3    position;
    float time_stamp;
};

struct KeyRotation {
    quat  orientation;
    float time_stamp;
};

struct KeyScale {
    v3    scale;
    float time_stamp;
};

struct Bone {
    id_ptr<Bone> parent;
    
    vector<KeyPosition> positions;
    vector<KeyRotation> rotations;
    vector<KeyScale>    scales;

    float time;
    m44 local_transform;

    m44 transform() const;
    void update(float new_time = -1.0f);
    m44 update_position();
    m44 update_rotation();
    m44 update_scaling();
};

struct SkeletonCPU {
    vector<id_ptr<Bone>> bones;
};

struct SkeletonGPU {
    vector<id_ptr<Bone>> bones;

    vuk::Unique<vuk::Buffer> buffer = vuk::Unique<vuk::Buffer>();

    bool render_lines = false;
    
    void update();

    static vuk::Unique<vuk::Buffer>* empty_buffer();
};

JSON_IMPL(KeyPosition, position, time_stamp);
JSON_IMPL(KeyRotation, orientation, time_stamp);
JSON_IMPL(KeyScale, scale, time_stamp);
JSON_IMPL(Bone, parent, positions, rotations, scales);

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
