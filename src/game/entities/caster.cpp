#include "caster.hpp"

#include <entt/core/hashed_string.hpp>

#include "extension/fmt.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/tags.hpp"

namespace spellbook {

using namespace entt::literals;

Caster::Caster(Scene* scene, entt::entity e) {
    damage = std::make_unique<Stat>(scene, e);
    heal = std::make_unique<Stat>(scene, e);
    attack_speed = std::make_unique<Stat>(scene, e);
    cooldown_speed = std::make_unique<Stat>(scene, e);
    projectile_speed = std::make_unique<Stat>(scene, e);
    range = std::make_unique<Stat>(scene, e);
    buff_duration = std::make_unique<Stat>(scene, e);
    debuff_duration = std::make_unique<Stat>(scene, e);
    lifesteal = std::make_unique<Stat>(scene, e);
    taunt = Taunt(scene);
}

bool Caster::casting() const {
    return (attack && attack->casting()) || (spell && spell->casting());
}


void caster_system(Scene* scene) {
    for (auto [entity, caster] : scene->registry.view<Caster>().each()) {
        if (caster.attack && !caster.casting() )
            caster.attack->targeting();
    }
    
    for (auto [entity, caster] : scene->registry.view<Caster>().each()) {
        Tags* tags = scene->registry.try_get<Tags>(entity);
        if (tags)
            if (tags->has_tag("no_cast"_hs))
                continue;

        if (caster.casting())
            continue;

        if (caster.attack && caster.attack->can_cast()) {
            caster.attack->request_cast();
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

bool Taunt::set(uint64 id, v3i pos) {
    type = Type_Position;
    source = id;
    position = pos;
    return true;
}
bool Taunt::set(uint64 id, entt::entity init_unit) {
    type = Type_UnitPosition;
    source = id;
    unit = init_unit;
    return true;
}
bool Taunt::reset(uint64 id) {
    if (source != 0 && source != id)
        return false;

    type = Type_Inactive;
    source = 0;
    return true;
}


}