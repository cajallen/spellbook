#include "asset_editor.hpp"

#include <tracy/Tracy.hpp>

#include "imgui/misc/cpp/imgui_stdlib.h"

#include "lib_ext/imgui_extra.hpp"

#include "input.hpp"


namespace spellbook {

void AssetEditor::setup() {
    ZoneScoped;
    p_scene = new Scene();
}

void AssetEditor::update() {
    ZoneScoped;
    if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->render_scene.query = v2i(Input::mouse_pos) - p_scene->render_scene.viewport.start;
    }
}

void AssetEditor::window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Asset Editor", p_open)) {
        PathSelect("File##Convert", &convert_file, "external_resources", "DND_PREFAB");
        if (ImGui::Button("Convert")) {
            save_prefab(convert_to_prefab(convert_file.string(), "prefabs", "prefab"));
        }
        ImGui::Separator();
        PathSelect("File##Load", &load_file, "resources", "DND_PREFAB");
        if (ImGui::Button("Load")) {
            prefab_cpu = load_prefab(load_file.string());
        }
        ImGui::Separator();
        inspect(&prefab_cpu);
        if (ImGui::Button("Instance")) {
            instance_prefab(p_scene->render_scene, prefab_cpu);
        }
    }
    ImGui::End();
}


}