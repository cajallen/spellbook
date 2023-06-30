#include "game/pose_controller.hpp"

#include <tracy/Tracy.hpp>

#include "renderer/assets/animation_state.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

void PoseController::set_state(AnimationState new_state, float time_in_target, float new_time_to_override) {
    if (state != new_state || animation == nullptr) {
        state = new_state;
        target_index = -1;
        fractional_state_total = time_in_target;
        time_to_next_index = 0.0f;
        time_to_override = new_time_to_override;

        animation = &skeleton.prefab->animations[new_state];
        if (animation->empty())
            animation = nullptr;
    }
}

bool PoseController::is_active() const {
    return animation != nullptr;
}

void PoseController::clear_state() {
    animation = nullptr;
}

void PoseController::progress_in_state() {
    if (animation == nullptr)
        return;
    
    target_index++;
    if (target_index >= animation->size()) {
        target_index = 0;
    }
    
    auto& target_frame = (*animation)[target_index];

    float used_timing = target_frame.time_to;
    switch (state) {
        case AnimationState_AttackInto:
        case AnimationState_AttackOut:
        case AnimationState_Attack2Into:
        case AnimationState_Attack2Out:
        case AnimationState_CastInto:
        case AnimationState_CastOut:
            used_timing = target_frame.time_to * (fractional_state_total == 0.0f ? 1.0f : fractional_state_total); 
    }
    if (time_to_override != -1.0f) {
        used_timing = time_to_override;
        time_to_override = -1.0f;
    }
    if (used_timing < 0.005f)
        used_timing = -1.0f;
    skeleton.load_frame(target_frame, used_timing);
    time_to_next_index = used_timing;
}


void PoseController::update(float delta_time) {
    if (animation == nullptr)
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