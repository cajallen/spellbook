#include "ability.hpp"

#include <imgui/imgui.h>

#include "components.hpp"
#include "extension/fmt.hpp"
#include "game/pose_controller.hpp"
#include "game/scene.hpp"
#include "game/entities/caster.hpp"

namespace spellbook {

Ability::~Ability() {
    if (pre_trigger_timer)
        remove_timer(scene, pre_trigger_timer);
    if (post_trigger_timer)
        remove_timer(scene, post_trigger_timer);
}


void Ability::request_cast() {
    if (auto_remove_dragging_impair)
        remove_dragging_impair(scene, caster);
    if (set_attack_anims || set_cast_anims) {
        auto poser = scene->registry.try_get<PoseController>(caster);
        if (poser) {
            poser->set_state(set_cast_anims ? AnimationState_CastInto : AnimationState_AttackInto, pre_trigger_time.value());
        }
    }
    
    start();
    pre_trigger_timer->start(pre_trigger_time.value());
}

bool Ability::casting() {
    return pre_trigger_timer->ticking || post_trigger_timer->ticking;
}

bool Ability::ready_to_cast() {
    return !casting();
}

void Ability::stop_casting() {
    pre_trigger_timer->stop();
    post_trigger_timer->stop();
}

void inspect(Ability* ability) {
    ImGui::PushID(ability);
    
    if (ability->has_target) {
        ImGui::DragInt3("Target", ability->target.data, 0.01f, 0, 0, "%d", ImGuiSliderFlags_NoInput);
    } else {
        ImGui::Text("No target");
    }
    
    if (ImGui::TreeNode("Pre Trigger Time")) {
        inspect(ability->pre_trigger_timer);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Post Trigger Time")) {
        inspect(ability->post_trigger_timer);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void Ability::setup(Scene* init_scene, entt::entity init_caster, float pre, float post, Type type) {
    scene = init_scene;
    caster = init_caster;
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    auto_remove_dragging_impair = true;
    set_attack_anims = type == Type_Attack;
    set_cast_anims = type == Type_Ability;
    pre_trigger_time = {&*caster_comp.attack_speed, pre};
    post_trigger_time = {&*caster_comp.attack_speed, post};
    pre_trigger_timer = &add_timer(scene, fmt_("{}::pre", name),
        [this](Timer* timer) {
            post_trigger_timer->start(post_trigger_time.value());
            if (set_cast_anims || set_attack_anims) {
                auto poser = scene->registry.try_get<PoseController>(caster);
                if (poser) {
                    poser->set_state(set_cast_anims ? AnimationState_CastOut : AnimationState_AttackOut, post_trigger_time.value());
                }
            }
            trigger();
        }, true
    );
    post_trigger_timer = &add_timer(scene, fmt_("{}::post", name),
    [this](Timer* timer) {
            end();
            if (set_cast_anims || set_attack_anims) {
                auto poser = scene->registry.try_get<PoseController>(caster);
                if (poser) {
                    poser->set_state(AnimationState_Idle);
                }
            }
        }, true
    );
}

void Ability::targeting() {

}

void Ability::start() {

}

void Ability::trigger() {

}

void Ability::end() {

}

float Ability::time_to_hit(v3i pos) {
    return pre_trigger_time.value();
}



}