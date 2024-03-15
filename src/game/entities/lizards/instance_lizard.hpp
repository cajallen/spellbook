#pragma once

#include "general/math/geometry.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/lizards/lizard.hpp"

namespace spellbook {

template <LizardType LT>
void instance_lizard(Scene* scene, v3i location) {
    static int i      = 0;
    auto& lizard_entry = get_lizard_database().lizard_entries[LT];
    auto       entity = setup_basic_unit(scene, lizard_entry.model_path, v3(location), 10.0f, lizard_entry.hurt_emitter);

    static int lizard_i = 0;
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", magic_enum::enum_name(LT).substr(strlen("LizardType_")), lizard_i++));

    scene->registry.emplace<Caster>(entity, scene, entity);
    scene->registry.emplace<Draggable>(entity, 0.5f);

    ModelTransform& model_tfm = scene->registry.get<ModelTransform>(entity);
    model_tfm.set_scale(v3(lizard_entry.scale));

    PoseController* poser = scene->registry.try_get<PoseController>(entity);
    if (poser)
        poser->set_state(AnimationState_Idle, 0.0f);
    Health& health = scene->registry.get<Health>(entity);
    health.regen = std::make_unique<Stat>(scene, entity, 0.0f);
}

}