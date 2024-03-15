#include "game/entities/lizards/lizard.hpp"

#include <entt/entity/entity.hpp>

#include "game/scene.hpp"
#include "game/entities/lizard.hpp"

namespace spellbook {

void build_laser_mage(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
}

}
