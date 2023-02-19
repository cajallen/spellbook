#include "game/pose_controller.hpp"

#include <tracy/Tracy.hpp>

#include "game/components.hpp"
#include "game/scene.hpp"

namespace spellbook {

void PoseController::set_state(State new_state, float time_in_target) {
    if (state != new_state) {
        state = new_state;
        target_index = -1;
        fractional_state_total = time_in_target;
        time_to_next_index = 0.0f;

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
        if (pose_set->entries.empty())
            pose_set = nullptr;
    }
}

void PoseController::clear_state() {
    state = State_Invalid;
    pose_set = nullptr;
}

void PoseController::progress_in_state() {
    if (pose_set == nullptr)
        return;
    
    target_index++;
    if (target_index >= pose_set->entries.size()) {
        target_index = 0;
    }
    
    auto& new_entry = pose_set->entries[target_index];

    float used_timing = new_entry.time_to;
    switch (state) {
        case State_Attacking:
        case State_Attacked:
            used_timing = new_entry.time_to * fractional_state_total; 
    }
    skeleton.load_pose(new_entry, used_timing);
    time_to_next_index = used_timing;
}


void PoseController::update(float delta_time) {
    if (pose_set == nullptr)
        return;
    
    time_to_next_index -= delta_time;
    if (time_to_next_index <= 0.0f) {
        progress_in_state();
    }
}


void pose_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, model, poser] : scene->registry.view<Model, PoseController>().each()) {
        auto& skeleton_cpu = *model.model_cpu->skeleton;
        skeleton_cpu.time += scene->delta_time;
        poser.update(scene->delta_time);
    }

    for (auto [entity, model] : scene->registry.view<Model>().each()) {
        if (model.model_cpu->skeleton)
            model.model_cpu->skeleton->update();
        if (model.model_gpu.skeleton)
            model.model_gpu.skeleton->update(*model.model_cpu->skeleton);
    }
}

}