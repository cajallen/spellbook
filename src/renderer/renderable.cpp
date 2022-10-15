#include "renderable.hpp"

#include <imgui.h>

#include "lib_ext/fmt_geometry.hpp"
#include "lib_ext/imgui_extra.hpp"

#include "matrix_math.hpp"

#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"

namespace spellbook {

void inspect(Renderable* renderable) {
    ImGui::PushID((void*) renderable);
    if (ImGui::TreeNode("Transform")) {
        DragMat4("Transform", &renderable->transform, 0.01f, "%.3f");
        ImGui::TreePop();
    }
    ImGui::Separator();
    ImGui::PopID();
}

}
