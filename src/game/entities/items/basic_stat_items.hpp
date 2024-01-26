#pragma once

#include <memory>
#include "game/scene.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/items/item.hpp"

namespace spellbook {

struct AmuletOfAgility : Item {
    void on_create() override {
        Item::on_create();
        name = "Amulet of Agility";
    }

    void on_equip(entt::entity equipper) override {
        Item::on_equip(equipper);

        Caster& caster = p_scene->registry.get<Caster>(equipper);

        if (!caster.attack_speed)
            caster.attack_speed = std::make_unique<Stat>(p_scene, equipper, 1.0f);

        caster.attack_speed->add_effect(hash_view(name), StatEffect{StatEffect::Type_Multiply, 1.2f}, nullptr);
    }

    void on_unequip() override {
        Item::on_unequip();

        Caster& caster = p_scene->registry.get<Caster>(equipper);
        caster.attack_speed->remove_effect(hash_view(name));
    }
};

}