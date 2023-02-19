#pragma once


#include "general/vector.hpp"
#include "general/id_ptr.hpp"

namespace spellbook {

struct Scene;
struct SkeletonCPU;
struct PoseSet;
struct SkeletonPrefab;

struct PoseController {
    enum State {
        State_Invalid,
        State_Idle,
        State_Flailing,
        State_Walking,
        State_Attacking,
        State_Attacked
    };
    
    SkeletonCPU& skeleton;
    
    State state;
    PoseSet* pose_set;
    
    int target_index = 0;
    float time_to_next_index = 0.0f;
    float fractional_state_total;
    
    void set_state(State new_state, float time_to_target, float time_in_target = 0.0f);
    void update(float delta_time);
    void progress_in_state();
};

void pose_system(Scene* scene);

}

