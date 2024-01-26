#pragma once

#include <utility>

#include "shop.hpp"

namespace spellbook {

struct Scene;

struct Bank {
    std::array<int, Bead_Count> beads;
};

struct Player {
    Scene* scene = nullptr;
    Bank bank = {};

    bool can_drag();
    Beads get_drag_cost();
};

void inspect(Bank* bank);

}