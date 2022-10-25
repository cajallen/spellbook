#include "asset_editor.hpp"

#include <tracy/Tracy.hpp>

#include "asset_browser.hpp"
#include "game.hpp"

#include "lib_ext/imgui_extra.hpp"

#include "input.hpp"

#include "game/components.hpp"

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
        if (ImGui::BeginTabBar("Asset Types")) {
            if (ImGui::BeginTabItem("Tower")) {
                auto tower_path = fs::path(tower_prefab.file_path);
                auto model_path = fs::path(tower_prefab.model_path);
                PathSelect("File", &tower_path, "resources", possible_tower, "DND_TOWER");
                PathSelect("Model", &model_path, "resources", possible_model, "DND_MODEL");
                tower_prefab.file_path = tower_path.string();
                tower_prefab.model_path = model_path.string();
                EnumCombo("Type", &tower_prefab.type);
        
                if (ImGui::Button("Save")) {
                    file_dump(from_jv<json>(to_jv(tower_prefab)), tower_prefab.file_path);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    string backup_path = tower_prefab.file_path;
                    tower_prefab = from_jv<TowerPrefab>(to_jv(parse_file(tower_prefab.file_path)));
                    tower_prefab.file_path = backup_path;
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    // static auto render_scene = new RenderScene();
    // static bool initialized = false;
    //
    // if (!initialized && !model_comp.model_cpu.file_name.empty()) {
    //     render_scene->name = "model_thumbnail";
    //     render_scene->viewport.name	 = render_scene->name + "::viewport";
    //     v3 cam_position = 4.f * v3(1, 1, 1);
    //     render_scene->viewport.camera = new Camera(cam_position, math::vector2euler(-cam_position));
    //     render_scene->viewport.setup();
    //
    //     ModelGPU model_gpu = instance_model(*render_scene, model_comp.model_cpu);
    //     
    //     game.renderer.add_scene(render_scene);
    //     initialized = true;
    // }
}


}