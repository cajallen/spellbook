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

    p_scene->render_scene.scene_data.ambient = Color(palette::white, 0.20);
    
    fs::path asset_editor_file = fs::path(game.user_folder) / ("asset_editor" + extension(FileType_General));
    
    json j = fs::exists(asset_editor_file) ? parse_file(asset_editor_file.string()) : json{};
    FROM_JSON_MEMBER(model_cpu.file_path)
    else
        model_cpu.file_path = "none";
    FROM_JSON_MEMBER(mesh_cpu.file_path)
    else
        mesh_cpu.file_path = "none";
    FROM_JSON_MEMBER(material_cpu.file_path)
    else
        material_cpu.file_path = "none";
    FROM_JSON_MEMBER(emitter_cpu.file_path)
    else
        emitter_cpu.file_path = "none";
    
    emitter_cpu.mesh = upload_mesh(generate_cube(v3(0.0f), v3(1.0f)));
    emitter_cpu.material = upload_material({.file_path = "emitter_mat", .color_tint = palette::black});
}

void AssetEditor::shutdown() {
    fs::path asset_editor_file = fs::path(game.user_folder) / ("asset_editor" + extension(FileType_General));
    fs::create_directories(asset_editor_file.parent_path());
    
    auto j = json();
    TO_JSON_MEMBER(model_cpu.file_path);
    TO_JSON_MEMBER(mesh_cpu.file_path);
    TO_JSON_MEMBER(material_cpu.file_path);
    TO_JSON_MEMBER(emitter_cpu.file_path);
    file_dump(j, asset_editor_file.string());
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
            p_scene->registry.destroy(showing_entity);
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
                deinstance_emitter(*emitter_gpu, false);
            emitter_gpu = nullptr;
        } break;
        default: break;
    }
    
    switch (new_tab) {
        case (Tab_Model): {
            showing_entity = p_scene->registry.create();
            p_scene->registry.emplace<Model>(showing_entity, model_cpu, instance_model(render_scene, model_cpu, false));
            p_scene->registry.emplace<ModelTransform>(showing_entity, v3(0.0f));
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
void asset_tab(AssetEditor& asset_editor, string name, AssetEditor::Tab type, T& asset_value, const std::function<void(bool)>& callback = {}) {
    if (ImGui::BeginTabItem(name.c_str())) {
        if (ImGui::Button("Reload##AssetTab") || asset_editor.tab != type)
            asset_editor.switch_tab(type);
                    
        bool changed = inspect(&asset_value);
                    
        if (ImGui::Button("Save##AssetTab")) {
            save_asset(asset_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##AssetTab")) {
            asset_value = load_asset<T>(asset_value.file_path);
        }
        if (callback)
            callback(changed);
        ImGui::EndTabItem();
    }
}

template<>
void asset_tab(AssetEditor& asset_editor, string name, AssetEditor::Tab type, ModelCPU& asset_value, const std::function<void(bool)>& callback) {
    if (ImGui::BeginTabItem(name.c_str())) {
        if (ImGui::Button("Reload##AssetTab") || asset_editor.tab != type)
            asset_editor.switch_tab(type);
        
        bool changed = inspect(&asset_value, m44::identity(), &asset_editor.p_scene->render_scene);
                    
        if (ImGui::Button("Save##AssetTab")) {
            save_asset(asset_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##AssetTab")) {
            asset_value = load_asset<ModelCPU>(asset_value.file_path);
        }
        if (callback)
            callback(changed);
        ImGui::EndTabItem();
    }
}

template<>
void asset_tab(AssetEditor& asset_editor, string name, AssetEditor::Tab type, EmitterCPU& asset_value, const std::function<void(bool)>& callback) {
    if (ImGui::BeginTabItem(name.c_str())) {
        if (ImGui::Button("Reload##AssetTab") || asset_editor.tab != type)
            asset_editor.switch_tab(type);
        
        bool changed = inspect(&asset_value);
                    
        if (ImGui::Button("Save##AssetTab")) {
            save_asset(asset_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##AssetTab")) {
            string old_mat = asset_value.material;
            asset_value = load_asset<EmitterCPU>(asset_value.file_path);
            asset_value.material = old_mat;
        }
        if (callback)
            callback(changed);
        ImGui::EndTabItem();
    }
}

void AssetEditor::window(bool* p_open) {
    ZoneScoped;

    if (ImGui::Begin("Asset Editor", p_open)) {
        if (ImGui::BeginTabBar("Asset Types")) {
            asset_tab(*this, "Model", Tab_Model, model_cpu, [this](bool changed) {
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
            asset_tab(*this, "Material", Tab_Material, material_cpu, [this](bool changed) {
                if (changed)
                    game.renderer.material_cache[hash_string(material_gpu->material_asset_path)].update_from_cpu(material_cpu);
            });
            asset_tab(*this, "Lizard", Tab_Lizard, lizard_prefab);
            asset_tab(*this, "Tile", Tab_Tile, tile_prefab);
            asset_tab(*this, "Enemy", Tab_Enemy, enemy_prefab);
            asset_tab(*this, "Spawner", Tab_Spawner, spawner_prefab);
            asset_tab(*this, "Consumer", Tab_Consumer, consumer_prefab);
            asset_tab(*this, "Emitter", Tab_Emitter, emitter_cpu, [this](bool changed) {
                if (changed)
                    emitter_gpu->update_from_cpu(emitter_cpu);
            });
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}


}
