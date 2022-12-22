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

ADD_EDITOR_SCENE(AssetEditor);

void AssetEditor::setup() {
    p_scene = new Scene();
    p_scene->setup("Asset Editor");
    p_scene->edit_mode = true;
    
    model_cpu.file_path = "none";
    mesh_cpu.file_path = "none";
    material_cpu.file_path = "none";
    emitter_cpu.file_path = "none";
    emitter_cpu.mesh = game.renderer.upload_mesh(generate_cube(v3(0.0f), v3(1.0f)));
    emitter_cpu.material = game.renderer.upload_material({.file_path = "emitter_mat"});
}

void AssetEditor::update() {
    ZoneScoped;
    
    if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->render_scene.query = v2i(Input::mouse_pos) - p_scene->render_scene.viewport.start;
    }

    p_scene->update();
}

void AssetEditor::switch_tab(Tab new_tab) {
    auto& render_scene = p_scene->render_scene;
    
    switch (tab) {
        case (Tab_Model): {
            deinstance_model(render_scene, *model_gpu);
            model_gpu.reset();
        } break;
        case (Tab_Mesh): {
            if (mesh_gpu != nullptr);
                render_scene.delete_renderable(mesh_gpu);
            mesh_gpu = nullptr;
        } break;
        case (Tab_Material): {
            if (material_gpu != nullptr);
                render_scene.delete_renderable(material_gpu);
            material_gpu = nullptr;
        } break;
        case (Tab_Lizard): {
        
        } break;
        case (Tab_Tile): {
        
        } break;
        case (Tab_Enemy): {
        
        } break;
        case (Tab_Spawner): {
        
        } break;
        case (Tab_Consumer): {
        
        } break;
        case (Tab_Emitter): {
            if (emitter_gpu != nullptr);
                render_scene.emitters.remove_index(render_scene.emitters.index(*emitter_gpu));
            emitter_gpu = nullptr;
        } break;
        default: break;
    }
    
    switch (new_tab) {
        case (Tab_Model): {
            model_gpu.emplace(instance_model(render_scene, model_cpu, false));
        } break;
        case (Tab_Mesh): {
            mesh_gpu = &render_scene.quick_mesh(mesh_cpu, false, false);
        } break;
        case (Tab_Material): {
            material_gpu = &render_scene.quick_material(material_cpu, false);
        } break;
        case (Tab_Lizard): {
        
        } break;
        case (Tab_Tile): {
        
        } break;
        case (Tab_Enemy): {
        
        } break;
        case (Tab_Spawner): {
        
        } break;
        case (Tab_Consumer): {
        
        } break;
        case (Tab_Emitter): {
            emitter_gpu = &instance_emitter(render_scene, emitter_cpu);
        } break;
        default: break;
    }
    tab = new_tab;
}

template<typename T>
void asset_tab(AssetEditor& asset_editor, string name, AssetEditor::Tab type, T& asset_value, const std::function<void()>& callback = {}) {
    if (ImGui::BeginTabItem(name.c_str())) {
        if (ImGui::Button("Reload") || asset_editor.tab != type)
            asset_editor.switch_tab(type);
                    
        inspect(&asset_value);
                    
        if (ImGui::Button("Save")) {
            save_asset(asset_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            asset_value = load_asset<T>(asset_value.file_path);
        }
        if (callback)
            callback();
        ImGui::EndTabItem();
    }
}

void AssetEditor::window(bool* p_open) {
    ZoneScoped;

    if (ImGui::Begin("Asset Editor", p_open)) {
        if (ImGui::BeginTabBar("Asset Types")) {
            asset_tab(*this, "Model", Tab_Model, model_cpu, [this]() {
                ImGui::SameLine();
                if (ImGui::Button("Split")) {
                    vector<ModelCPU> models = model_cpu.split();
                    for (auto& model : models) {
                        fs::path base_path = fs::path(model_cpu.file_path).parent_path();
                        model.file_path = (base_path / (model.file_path + extension(FileType_Model))).string();
                        save_asset(model);
                    }
                    model_cpu = models.front();
                }
            });
            asset_tab(*this, "Material", Tab_Material, material_cpu);
            asset_tab(*this, "Lizard", Tab_Lizard, lizard_prefab);
            asset_tab(*this, "Tile", Tab_Tile, tile_prefab);
            asset_tab(*this, "Enemy", Tab_Enemy, enemy_prefab);
            asset_tab(*this, "Spawner", Tab_Spawner, spawner_prefab);
            asset_tab(*this, "Consumer", Tab_Consumer, consumer_prefab);
            asset_tab(*this, "Emitter", Tab_Emitter, emitter_cpu);
            
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
