#include "game/pose_controller.hpp"

#include <tracy/Tracy.hpp>

#include "game/components.hpp"
#include "game/scene.hpp"

namespace spellbook {

void PoseController::set_state(State new_state, float time_to_target, float time_in_target) {
    if (state != new_state) {
        state = new_state;
        target_index = -1;
        time_to_next_index = time_to_target;
        fractional_state_total = time_in_target;

        switch (new_state) {
            case State_Idle: {
                pose_set = &skeleton.prefab->poses[PoseSet::Type_Idle];
            } break;
            case State_Flailing: {
                pose_set = &skeleton.prefab->poses[PoseSet::Type_Flail];
            } break;
            default: {
                pose_set = &skeleton.prefab->poses[PoseSet::Type_Idle];
            } break;
        }
    }
}

void PoseController::progress_in_state() {
    target_index++;
    if (target_index >= pose_set->ordering.size()) {
        target_index = 0;
    }
    
    string new_name = pose_set->ordering[target_index];

    float used_timing = pose_set->timings[new_name];
    switch (state) {
        case State_Attacking:
        case State_Attacked:
            used_timing = pose_set->timings[new_name] * fractional_state_total; 
    }
    skeleton.load_pose(*pose_set, new_name, used_timing);
    time_to_next_index = used_timing;
}


void PoseController::update(float delta_time) {
    if (target_index == -1) {
        skeleton.load_pose(*pose_set, pose_set->ordering.front(), time_to_next_index);
    }
    
    time_to_next_index -= delta_time;
    if (time_to_next_index <= 0.0f) {
        progress_in_state();
    }
}


void pose_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, model, poser] : scene->registry.view<Model, PoseController>().each()) {
        auto& skeleton_cpu = *model.model_cpu.skeleton;
        skeleton_cpu.time += scene->delta_time;
        poser.update(scene->delta_time);
    }

    for (auto [entity, model] : scene->registry.view<Model>().each()) {
        if (model.model_cpu.skeleton)
            model.model_cpu.skeleton->update();
        if (model.model_gpu.skeleton)
            model.model_gpu.skeleton->update(*model.model_cpu.skeleton);
    }
}

}