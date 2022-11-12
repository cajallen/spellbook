#include "renderable.hpp"

#include <imgui.h>

#include "lib_ext/imgui_extra.hpp"

namespace spellbook {

void inspect(Renderable* renderable) {
    ImGui::PushID((void*) renderable);
    if (ImGui::TreeNode("Transform")) {
        ImGui::DragMat4("Transform", &renderable->transform, 0.01f, "%.3f");
        ImGui::TreePop();
    }
    ImGui::Separator();
    ImGui::PopID();
}

}
