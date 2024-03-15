#include "ability.hpp"

#include <imgui/imgui.h>
#include <entt/core/hashed_string.hpp>

#include "extension/fmt.hpp"
#include "game/pose_controller.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/tags.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/lizards/lizard.hpp"

namespace spellbook {

using namespace entt::literals;

Ability::Ability(Scene* _scene, entt::entity _caster) : scene(_scene), caster(_caster) {}
Attack::Attack(Scene* _scene, entt::entity _caster) : Ability(_scene, _caster) {}
Spell::Spell(Scene* _scene, entt::entity _caster) : Ability(_scene, _caster) {}

void Ability::setup_time(float base_pre, float base_post) {
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    pre_trigger_time = {&*caster_comp.attack_speed, base_pre};
    post_trigger_time = {&*caster_comp.attack_speed, base_post};
    pre_trigger_timer = add_timer(scene,
        [this](Timer* timer) {
            post_trigger_timer->start(post_trigger_time.value());
            if (set_anims)
                if (auto poser = scene->registry.try_get<PoseController>(caster))
                    poser->set_state(get_post_trigger_animation_state(), post_trigger_time.value());
            trigger();
        }, false
    );
    post_trigger_timer = add_timer(scene,
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

void Attack::setup_cd(float base_cd) {
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    cooldown_time = {&*caster_comp.cooldown_speed, base_cd};
    cooldown_timer = add_timer(scene, [](Timer* timer) {});

    pre_trigger_timer = add_timer(scene,
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

bool Ability::can_cast() const {
    Tags& tags = scene->registry.get<Tags>(caster);
    if (tags.has_tag("no_cast"_hs))
        return false;
    if (!has_target)
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