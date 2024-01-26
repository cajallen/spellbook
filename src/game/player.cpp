#include "player.hpp"

#include "game/scene.hpp"
#include "game/entities/spawner.hpp"

#include <imgui/imgui.h>

namespace spellbook {

bool Player::can_drag() {
    Beads cost = get_drag_cost();
    return bank.beads[cost.type] >= cost.amount;
}

Beads Player::get_drag_cost() {
    if (scene->spawn_state_info->round_active)
        return {Bead_Quartz, 1};
    return {Bead_Quartz, 0};
}

void inspect(Bank* bank) {
    for (uint32 i = 0; i < bank->beads.size(); i++) {
        auto bead_name = string(magic_enum::enum_name((Bead) i));
        ImGui::InputInt(bead_name.c_str(), &bank->beads[i]);
    }
}

}
