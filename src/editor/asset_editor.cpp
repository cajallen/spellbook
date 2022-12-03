#include "asset_editor.hpp"

#include <tracy/Tracy.hpp>
#include <vuk/Partials.hpp>

#include "extension/icons/font_awesome4.h"
#include "extension/imgui_extra.hpp"
#include "game/game.hpp"
#include "game/input.hpp"
#include "game/components.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

void AssetEditor::setup() {
    ZoneScoped;
    p_scene = new Scene();
    p_scene->setup("Asset Editor");
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
            if (ImGui::BeginTabItem("Material")) {
                tab = Tab_Material;
                inspect(&material_cpu);
                
                if (ImGui::Button("Save")) {
                    file_dump(from_jv<json>(to_jv(material_cpu)), material_cpu.file_path);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    string backup_path = material_cpu.file_path;
                    material_cpu = from_jv<MaterialCPU>(to_jv(parse_file(material_cpu.file_path)));
                    material_cpu.file_path = backup_path;
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Model")) {
                tab = Tab_Model;
                inspect(&model_cpu);
                
                if (ImGui::Button("Save")) {
                    save_model(model_cpu);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    model_cpu = load_model(model_cpu.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Tower")) {
                tab = Tab_Tower;
                inspect(&tower_prefab);
                
                if (ImGui::Button("Save")) {
                    save_tower(tower_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    tower_prefab = load_tower(tower_prefab.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Tile")) {
                tab = Tab_Tile;
                inspect(&tile_prefab);
                
                if (ImGui::Button("Save")) {
                    save_tile(tile_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    tile_prefab = load_tile(tile_prefab.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Enemy")) {
                tab = Tab_Enemy;
                inspect(&enemy_prefab);
                
                if (ImGui::Button("Save")) {
                    save_enemy(enemy_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    enemy_prefab = load_enemy(enemy_prefab.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Spawner")) {
                tab = Tab_Spawner;
                inspect(&spawner_prefab);
                
                if (ImGui::Button("Save")) {
                    save_spawner(spawner_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    spawner_prefab = load_spawner(spawner_prefab.file_path);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Consumer")) {
                tab = Tab_Consumer;
                inspect(&consumer_prefab);
                
                if (ImGui::Button("Save")) {
                    save_consumer(consumer_prefab);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    consumer_prefab = load_consumer(consumer_prefab.file_path);
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Emitter")) {
                tab = Tab_Emitter;
                inspect(emitter_instance);
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
