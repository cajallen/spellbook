#pragma once

#include <entt/entity/entity.hpp>
#include "general/string.hpp"

namespace spellbook {

struct Scene;

struct Item {
    Scene* p_scene;
    entt::entity equipper;
    string name;

    virtual void on_create() {

    }
    virtual void on_equip(entt::entity new_equipper) {
        equipper = new_equipper;
    }
    virtual void on_unequip() {
        equipper = entt::null;
    }
};

}