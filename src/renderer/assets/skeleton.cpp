#include "skeleton.hpp"

#include <vuk/Buffer.hpp>

#include "lib/matrix_math.hpp"
#include "game/input.hpp"
#include "game/game.hpp"

namespace spellbook {

m44 Bone::transform() const {
    if (parent.valid())
        return parent->transform() * local_transform;
    return local_transform;
}

void Bone::update(float new_time) {
    if (new_time == -1.0f)
        time = Input::time;
    else
        time = new_time;
    
    m44 translation = update_position();
    m44 rotation    = update_rotation();
    m44 scale       = update_scaling();
    local_transform  = translation * rotation * scale;
}

m44 Bone::update_position() {
    if (positions.empty())
        return m44::identity();
    auto [position1, time_stamp1] = positions.front();
    auto end_it = positions.begin();
    while (end_it->time_stamp < time) {
        if (end_it == positions.end()) {
            return math::translate(positions.back().position);
        }
        ++end_it;
    }
    auto [position2, time_stamp2] = *end_it;
    v3 interpolated = math::mix(position1, position2, (time_stamp2 - time_stamp1) != 0.0f ? (time - time_stamp1) / (time_stamp2 - time_stamp1) : 0.0f);
    return math::translate(interpolated);
}

m44 Bone::update_rotation() {
    if (rotations.empty())
        return m44::identity();
    auto [orientation1, time_stamp1] = rotations.front();
    auto end_it = rotations.begin();
    while (end_it->time_stamp < time) {
        if (end_it == rotations.end()) {
            return math::rotation(rotations.back().orientation);
        }
        ++end_it;
    }
    auto [orientation2, time_stamp2] = *end_it;
    float t = (time_stamp2 - time_stamp1) != 0.0f ? (time - time_stamp1) / (time_stamp2 - time_stamp1) : 0.0f;
    quat interpolated = math::slerp(orientation1, orientation2, t);
    return math::rotation(interpolated);
}

m44 Bone::update_scaling() {
    if (scales.empty())
        return m44::identity();
    auto [scale1, time_stamp1] = scales.front();
    auto end_it = scales.begin();
    while (end_it->time_stamp < time) {
        if (end_it == scales.end()) {
            return math::scale(scales.back().scale);
        }
        ++end_it;
    }
    auto [scale2, time_stamp2] = *end_it;
    v3 interpolated = math::mix(scale1, scale2, (time_stamp2 - time_stamp1) != 0.0f ? (time - time_stamp1) / (time_stamp2 - time_stamp1) : 0.0f);
    return math::scale(interpolated);
}

void SkeletonGPU::update() {
    for (auto bone_ptr : bones) {
        bone_ptr->update();
    }

    vector<u8> bones_data( sizeof(u32) * 4 + sizeof(m44GPU) * bones.size());
    bones_data.append_data(bones.size());
    struct { u32 a,b,c; } padding;
    bones_data.append_data(padding);
    for (id_ptr<Bone> bone : bones) {
        bones_data.append_data((m44GPU) bone->transform());
    }
    memcpy(buffer->mapped_ptr, bones_data.data(), bones_data.size());
}

vuk::Unique<vuk::Buffer>* SkeletonGPU::empty_buffer() {
    static SkeletonGPU skeleton_gpu = game.renderer.upload_skeleton(SkeletonCPU{});
    return &skeleton_gpu.buffer;
}
}
