#include "player.hpp"

#include <imgui/imgui.h>

namespace spellbook {

void inspect(Bank* bank) {
    for (u32 i = 0; i < bank->beads.size(); i++) {
        auto bead_name = string(magic_enum::enum_name((Bead) i));
        ImGui::InputInt(bead_name.c_str(), &bank->beads[i]);
    }
}

}
