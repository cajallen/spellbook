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
};

void inspect(Bank* bank);

}