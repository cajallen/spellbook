#include "light.hpp"

#include <imgui.h>

#include "extension/imgui_extra.hpp"

namespace spellbook {

void inspect(DirectionalLight* light) {
    ImGui::DragEuler2("Direction", &light->dir);
    ImGui::ColorEdit4("Color", light->color.data);
}

}
