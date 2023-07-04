#include "ability.hpp"

#include <imgui/imgui.h>

#include "components.hpp"
#include "impair.hpp"
#include "extension/fmt.hpp"
#include "game/pose_controller.hpp"
#include "game/scene.hpp"
#include "game/entities/caster.hpp"

namespace spellbook {

Ability::Ability(Scene* init_scene, entt::entity init_caster, float pre, float post, float init_range) {
    scene = init_scene;
    caster = init_caster;
    Caster& caster_comp = scene->registry.get<Caster>(caster);

    range = {&*caster_comp.range, init_range};
    
    pre_trigger_time = {&*caster_comp.attack_speed, pre};
    post_trigger_time = {&*caster_comp.attack_speed, post};
    pre_trigger_timer = add_timer(scene, fmt_("{}::pre", get_name()),
        [this](Timer* timer) {
            post_trigger_timer->start(post_trigger_time.value());
            if (set_anims)
                if (auto poser = scene->registry.try_get<PoseController>(caster))
                    poser->set_state(get_post_trigger_animation_state(), post_trigger_time.value());
            trigger();
        }, false
    );
    post_trigger_timer = add_timer(scene, fmt_("{}::post", get_name()),
        [this](Timer* timer) {
            end();
            if (set_anims) {
                auto poser = scene->registry.try_get<PoseController>(caster);
                if (poser) {
                    poser->set_state(AnimationState_Idle);
                }
            }
        }, false
    );
}

Attack::Attack(Scene* init_scene, entt::entity init_caster, float pre, float post, float cooldown, float cast_range) : Ability(init_scene, init_caster, pre, post, cast_range) {
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    cooldown_time = {&*caster_comp.cooldown_reduction, cooldown};
    cooldown_timer = add_timer(scene, fmt_("{}::cd", get_name()), [this](Timer* timer) {});

    pre_trigger_timer = add_timer(scene, fmt_("{}::pre", get_name()),
        [this](Timer* timer) {
            post_trigger_timer->start(post_trigger_time.value());
            cooldown_timer->start(cooldown_time.value());
            if (set_anims)
                if (auto poser = scene->registry.try_get<PoseController>(caster))
                    poser->set_state(get_post_trigger_animation_state(), post_trigger_time.value());
            trigger();
        }, false
    );
}

void Ability::request_cast() {
    if (set_anims) {
        auto poser = scene->registry.try_get<PoseController>(caster);
        if (poser) {
            poser->set_state(get_pre_trigger_animation_state(), pre_trigger_time.value());
        }
    }
    
    start();
    pre_trigger_timer->start(pre_trigger_time.value());
}

void Spell::request_cast() {
    remove_dragging_impair(scene, caster);
    Ability::request_cast();
}


bool Ability::casting() const {
    return pre_trigger_timer->ticking || post_trigger_timer->ticking;
}

void Ability::stop_casting() {
    pre_trigger_timer->stop();
    post_trigger_timer->stop();
}

void inspect(Ability* ability) {
    ImGui::PushID(ability);
    ImGui::PopID();
}

float Ability::time_to_hit(v3i pos) {
    return pre_trigger_time.value();
}


void Ability::lizard_turn_to_target() {
    v3i caster_pos = math::round_cast(scene->registry.get<LogicTransform>(caster).position);
    auto lizard = scene->registry.try_get<Lizard>(caster);
    if (lizard) {
        v3 dir_to = math::normalize(v3(target) - v3(caster_pos));
        float ang = math::angle_difference(lizard->default_direction.xy, dir_to.xy);
        scene->registry.get<LogicTransform>(caster).yaw = ang;
    }
}

bool Ability::in_range() const {
    LogicTransform& logic_tfm = scene->registry.get<LogicTransform>(caster);
    bool out_of_range = math::abs(float(target.z) - logic_tfm.position.z) > 0.2f ||
        math::abs(float(target.x) - logic_tfm.position.x) > range.value() ||
        math::abs(float(target.y) - logic_tfm.position.y) > range.value();
    return !out_of_range;
}

bool Ability::can_cast() const {
    Impairs& impairs = scene->registry.get<Impairs>(caster);
    if (impairs.is_impaired(scene, ImpairType_NoCast))
        return false;
    if (!has_target)
        return false;
    
    if (!in_range())
        return false;
    if (casting())
        return false;
    return true;
}

bool Attack::can_cast() const {
    if (cooldown_timer && cooldown_timer->ticking)
        return false;
    return Ability::can_cast();
}





}