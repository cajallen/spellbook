﻿#include "caster.hpp"

#include "components.hpp"
#include "extension/fmt.hpp"
#include "game/scene.hpp"
#include "game/entities/impair.hpp"

namespace spellbook {

Caster::Caster(Scene* scene) {
    damage = std::make_unique<Stat>(scene);
    heal = std::make_unique<Stat>(scene);
    attack_speed = std::make_unique<Stat>(scene);
    cooldown_reduction = std::make_unique<Stat>(scene);
    projectile_speed = std::make_unique<Stat>(scene);
    range = std::make_unique<Stat>(scene);
    buff_duration = std::make_unique<Stat>(scene);
    debuff_duration = std::make_unique<Stat>(scene);
    lifesteal = std::make_unique<Stat>(scene);
    taunt = Taunt(scene);
}

bool Caster::casting() const {
    return (attack && attack->casting()) || (ability && ability->casting());
}


void caster_system(Scene* scene) {
    for (auto [entity, caster] : scene->registry.view<Caster>().each()) {
        v3i taunt_target;
        if (caster.taunt.try_get(&taunt_target)) {
            caster.attack->target = taunt_target;
            caster.attack->has_target = true;
            continue;
        }
        if (!caster.casting())
            caster.attack->targeting();
    }

    for (auto [entity, caster] : scene->registry.view<Caster>().each()) {
        Impairs* impairs = scene->registry.try_get<Impairs>(entity);
        if (impairs)
            if (impairs->is_impaired(ImpairType_NoCast))
                continue;

        if (caster.casting())
            continue;

        // TODO: need range check on here
        if (caster.attack->ready_to_cast()) {
            if (caster.attack->has_target) {
                caster.attack->request_cast();
            }
        }
    }
}

bool Taunt::try_get(v3i* pos) {
    if (type == Type_Inactive)
        return false;
    if (type == Type_UnitPosition) {
        if (!scene->registry.valid(unit)) {
            type = Type_Inactive;
            return false;
        }
        *pos = math::round_cast(scene->registry.get<LogicTransform>(unit).position);
        return true;
    }
    if (type == Type_Position) {
        *pos = position;
        return true;
    }

    log_error("unreachable");
    return false;
}

bool Taunt::set(u64 id, v3i pos) {
    type = Type_Position;
    source = id;
    position = pos;
    return true;
}
bool Taunt::set(u64 id, entt::entity init_unit) {
    type = Type_UnitPosition;
    source = id;
    unit = init_unit;
    return true;
}
bool Taunt::reset(u64 id) {
    if (source != 0 && source != id)
        return false;

    type = Type_Inactive;
    source = 0;
    return true;
}


}