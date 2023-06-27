#pragma once

#include "renderer/assets/animation_state.hpp"

namespace spellbook {

struct Scene;
struct SkeletonCPU;
struct PoseSet;
struct SkeletonPrefab;

struct PoseController {
    SkeletonCPU& skeleton;
    
    AnimationState state = AnimationState_Idle;
    PoseSet* pose_set = nullptr;
    
    int target_index = 0;
    float time_to_next_index = 0.0f;
    float fractional_state_total = 0.0f;

    float time_to_override = -1.0f;

    bool is_active() const;
    void set_state(AnimationState new_state, float time_in_target = 0.0f, float time_to_override = -1.0f);
    void clear_state();
    void update(float delta_time);
    void progress_in_state();
};

void pose_system(Scene* scene);

}

