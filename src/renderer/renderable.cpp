#include "renderable.hpp"

#include <imgui.h>

#include "lib_ext/fmt_geometry.hpp"
#include "lib_ext/imgui_extra.hpp"

#include "matrix_math.hpp"

#include "render_scene.hpp"

namespace spellbook {

void inspect(Renderable* renderable) {
    if (ImGui::TreeNode("Transform")) {
        DragMat4("Transform", &renderable->transform, 0.01f, "%.3f");
        ImGui::TreePop();
    }
    ImGui::Separator();
}

}
