#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizard.hpp"

namespace spellbook {

struct IllusionAttack : Ability {
    void targeting() override;
    void trigger() override;
};

void IllusionAttack::trigger() {
    quick_emitter(scene, "Illusion Basic", v3(target), "emitters/shadow_daggers.sbemt", 0.2f);
    add_timer(scene, "Illusion Basic Hurt Timer", [this](Timer* timer) {
        auto enemies = entry_gather_function(*this, target, 0.0f);
        for (auto& enemy : enemies) {
            auto& health = scene->registry.get<Health>(enemy);
            damage(scene, caster, enemy, 2.5f, v3(0, 0, -1));
        }
    }, false).start(0.3f);
}

void IllusionAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    
    if (taunted(*this, caster_comp))
        return;

    if (square_targeting(3, *this, entry_gather_function))
        return;
}

void build_illusion_mage(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    Caster& caster = scene->registry.get<Caster>(entity);

    caster.attack = std::make_unique<IllusionAttack>();
    caster.attack->setup(scene, entity, 0.9f, 0.5f, Ability::Type_Attack);
    caster.attack->entry_gather_function = lizard_entry_gather();
}

}