#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "game/scene.hpp"
#include "game/entities/lizard.hpp"

namespace spellbook {

void build_treasurer(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
}

}
