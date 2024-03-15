#include "resource_editor.hpp"

#include <tracy/Tracy.hpp>
#include <vuk/Partials.hpp>
#include <entt/entity/registry.hpp>

#include "extension/icons/font_awesome4.h"
#include "extension/imgui_extra.hpp"
#include "extension/fmt.hpp"
#include "general/file/file_path.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "editor/pose_widget.hpp"
#include "game/game.hpp"
#include "general/input.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/caster.hpp"

namespace spellbook {

//ADD_EDITOR_SCENE(ResourceEditor);

void ResourceEditor::set_model(ModelCPU* model, Ownership ownership) {
    if (model_owner == Ownership_Owned)
        delete model_cpu;
    
    model_cpu = model;
    model_owner = ownership;
}


void ResourceEditor::setup() {
    p_scene = new Scene();
    p_scene->setup("Resource Editor");
    p_scene->set_edit_mode(true);

    p_scene->render_scene.scene_data.ambient = Color(palette::white, 0.20f);

    FilePath config_path = "resource_editor.sbjgen"_config;

    set_model(new ModelCPU(), Ownership_Owned);
    
    json j = fs::exists(config_path.abs_path()) ? parse_file(config_path.abs_string()) : json{};
    if (j.contains("tab"))
        external_tab_selection = from_jv<Tab>(*j.at("tab"));
    FROM_JSON_MEMBER(model_cpu->file_path)
    else
        model_cpu->file_path = FilePath();
    FROM_JSON_MEMBER(mesh_cpu.file_path)
    else
        mesh_cpu.file_path = FilePath();
    FROM_JSON_MEMBER(material_cpu.file_path)
    else
        material_cpu.file_path = FilePath();
    FROM_JSON_MEMBER(emitter_cpu.file_path)
    else
        emitter_cpu.file_path = FilePath();
    FROM_JSON_MEMBER(tile_set.file_path)
    else
        tile_set.file_path = FilePath();
    FROM_JSON_MEMBER(bead_prefab.file_path)
    else
        bead_prefab.file_path = FilePath();
    FROM_JSON_MEMBER(enemy_prefab.file_path)
    else
        enemy_prefab.file_path = FilePath();
    

    MeshCPU cube_mesh = generate_cube(v3(0.0f), v3(1.0f));
    emitter_cpu.mesh = cube_mesh.file_path;
    upload_mesh(cube_mesh);
    MaterialCPU emitter_material = {.color_tint = palette::black};
    emitter_material.file_path = "emitter_mat"_symbolic;
    emitter_cpu.material = emitter_material.file_path;

    upload_material(emitter_material);
}

void ResourceEditor::shutdown() {
   FilePath config_path = "resource_editor.sbjgen"_config;
   fs::create_directories(config_path.parent_path());
    
    auto j = json();
    TO_JSON_MEMBER(tab);
    TO_JSON_MEMBER(model_cpu->file_path);
    TO_JSON_MEMBER(mesh_cpu.file_path);
    TO_JSON_MEMBER(material_cpu.file_path);
    TO_JSON_MEMBER(emitter_cpu.file_path);
    TO_JSON_MEMBER(tile_set.file_path);
    TO_JSON_MEMBER(bead_prefab.file_path);
    TO_JSON_MEMBER(enemy_prefab.file_path);
    file_dump(j, config_path.abs_string());
    p_scene->cleanup();
}

void ResourceEditor::update() {
    ZoneScoped;
    
    if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->render_scene.query = v2i(Input::mouse_pos) - p_scene->render_scene.viewport.start;
    }

    p_scene->update();
}

void ResourceEditor::switch_tab(Tab new_tab) {
    auto& render_scene = p_scene->render_scene;

    FilePath model_path = model_cpu->file_path;
    p_scene->registry.clear();
    if (model_owner == Ownership_From_Instance) {
        set_model(new ModelCPU(), Ownership_Owned);
        model_cpu->file_path = model_path;
    }
    
    switch (tab) {
        case (Tab_Model): {
        } break;
        case (Tab_Mesh): {
            if (mesh_preview != nullptr)
                render_scene.delete_renderable(mesh_preview);
            mesh_preview = nullptr;
        } break;
        case (Tab_Material): {
            if (material_preview != nullptr)
                render_scene.delete_renderable(material_preview);
            material_preview = nullptr;
        } break;
        case (Tab_Tile): {
        
        } break;
        case (Tab_Enemy): {
            if (background_renderable != nullptr)
                render_scene.delete_renderable(background_renderable);
        } break;
        case (Tab_Spawner): {
        
        } break;
        case (Tab_Consumer): {
        
        } break;
        case (Tab_Emitter): {
            if (emitter_gpu != nullptr)
                deinstance_emitter(*emitter_gpu, false);
            emitter_gpu = nullptr;
        } break;
        case (Tab_TileSet): {
            p_scene->render_scene.render_grid = false;
        } break;
        case (Tab_Drop): {
        } break;
        default: break;
    }
    
    switch (new_tab) {
        case (Tab_Model): {
            entt::entity showing_entity = p_scene->registry.create();
            Model* model;
            model = &p_scene->registry.emplace<Model>(showing_entity, std::make_unique<ModelCPU>(*model_cpu), instance_model(render_scene, *model_cpu, false));

            set_model(&*model->model_cpu, Ownership_From_Instance);
            p_scene->registry.emplace<ModelTransform>(showing_entity, v3(0.0f));
            if (model->model_cpu->skeleton) {
                p_scene->registry.emplace<PoseController>(showing_entity, *model->model_cpu->skeleton);
            }
        } break;
        case (Tab_Mesh): {
            mesh_preview = &render_scene.quick_mesh(mesh_cpu, false, false);
        } break;
        case (Tab_Material): {
            material_preview = &render_scene.quick_material(material_cpu, false);
        } break;
        case (Tab_Tile): {
            if (tile_prefab.file_path.is_file())
                instance_prefab(p_scene, tile_prefab, v3i(0));
        } break;
        case (Tab_Enemy): {
            MeshCPU cube = generate_cube(v3(0.5f, 0.5f, -1.0f), v3(3.0f, 3.0f, 1.0f));
            MaterialCPU mat = MaterialCPU{.color_tint = palette::gray};
            mat.file_path = "background_mat"_symbolic;
            uint64 mat_id = upload_material(mat);
            background_renderable = &p_scene->render_scene.quick_renderable(cube, mat_id, false);
            
            if (enemy_prefab.file_path.is_file())
                instance_prefab(p_scene, enemy_prefab, v3i(0));
        } break;
        case (Tab_Spawner): {
        
        } break;
        case (Tab_Consumer): {
        
        } break;
        case (Tab_Emitter): {
            emitter_gpu = &instance_emitter(render_scene, emitter_cpu, Input::time);
        } break;
        case (Tab_TileSet): {
            auto vtsw_entity = p_scene->registry.create();
            p_scene->registry.emplace<Name>(vtsw_entity, fmt_("vtsw"));
            p_scene->registry.emplace<VisualTileSetWidget>(vtsw_entity, &tile_set);
            
            uint32 width = uint32(math::ceil(math::sqrt(float(tile_set.tiles.size()))));
            uint32 i = 0;
            for (VisualTilePrefab& tile_entry : tile_set.tiles) {
                auto entity = p_scene->registry.create();
                v3 pos = (v3(i % width, i / width, 0.0f) - v3(0.5f * width, 0.5f * width, 0.0f)) * 3.0f;
                i++;
                p_scene->registry.emplace<Name>(entity, fmt_("tile:({},{},{})",pos.x,pos.y,pos.z));
                
                auto& model_comp = p_scene->registry.emplace<Model>(entity);
                model_comp.model_cpu = std::make_unique<ModelCPU>(load_resource<ModelCPU>(tile_entry.model_path));
                model_comp.model_gpu = instance_model(p_scene->render_scene, *model_comp.model_cpu);
                
                p_scene->registry.emplace<ModelTransform>(entity, v3(pos));
            }
            p_scene->render_scene.render_grid = false;
        } break;
        case (Tab_Drop): {
            instance_prefab(p_scene, bead_prefab, v3(0.0f));
        } break;
        default: break;
    }
    tab = new_tab;
}

template<typename T>
void asset_tab(
    ResourceEditor& asset_editor, string name, ResourceEditor::Tab type, T* asset_value, const std::function<void(bool)>& callback = {}) {
    if (ImGui::BeginTabItem(name.c_str(), nullptr, type == asset_editor.external_tab_selection ? ImGuiTabItemFlags_SetSelected : 0)) {
        if (ImGui::Button("Save##ResourceTab")) {
            save_resource(*asset_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##ResourceTab")) {
            *asset_value = load_resource<T>(asset_value->file_path, false, true);
            asset_editor.switch_tab(type);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload##ResourceTab") || asset_editor.tab != type)
            asset_editor.switch_tab(type);
        bool changed = inspect(asset_value);
                    
        if (callback)
            callback(changed);
        ImGui::EndTabItem();
    }
}

template<>
void asset_tab(ResourceEditor& asset_editor, string name, ResourceEditor::Tab type, ModelCPU** asset_value, const std::function<void(bool)>& callback) {
    if (ImGui::BeginTabItem(name.c_str(), nullptr, type == asset_editor.external_tab_selection ? ImGuiTabItemFlags_SetSelected : 0)) {
        if (ImGui::Button("Reload##ResourceTab") || asset_editor.tab != type) {
            asset_editor.switch_tab(type);
        }
        
        bool changed = inspect(*asset_value, &asset_editor.p_scene->render_scene);
        if (ImGui::Button("Save##ResourceTab")) {
            save_resource(**asset_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##ResourceTab")) {
            asset_editor.set_model(&load_resource<ModelCPU>((*asset_value)->file_path, false, true), ResourceEditor::Ownership_From_Cache);
            asset_editor.switch_tab(type);
        }
        ImGui::SameLine();
        if (ImGui::Button("Split")) {
            // TODO: verify memory
            vector<ModelCPU> models = (*asset_value)->split();
            for (auto& model : models) {
                // TODO
                sb_assert(false && "NYI");
                fs::path base_path = (*asset_value)->file_path.abs_path().parent_path();
//                model.file_path = (base_path / (model.file_path + extension(FileType_Model))).string();
//                save_resource(model);
            }
            // asset_value = models.front();
        }
        if (callback)
            callback(changed);
        ImGui::EndTabItem();
    }
}

template<>
void asset_tab(ResourceEditor& asset_editor, string name, ResourceEditor::Tab type, EmitterCPU* asset_value, const std::function<void(bool)>& callback) {
    if (ImGui::BeginTabItem(name.c_str(), nullptr, type == asset_editor.external_tab_selection ? ImGuiTabItemFlags_SetSelected : 0)) {
        if (ImGui::Button("Reload##ResourceTab") || asset_editor.tab != type)
            asset_editor.switch_tab(type);
        
        bool changed = inspect(asset_editor.p_scene->render_scene, asset_value);
                    
        if (ImGui::Button("Save##ResourceTab")) {
            save_resource(*asset_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##ResourceTab")) {
            FilePath old_mat = asset_value->material;
            *asset_value = load_resource<EmitterCPU>(asset_value->file_path, false, true);
            asset_value->material = old_mat;
            asset_editor.switch_tab(type);
        }
        if (callback)
            callback(changed);
        ImGui::EndTabItem();
    }
}

template<>
void asset_tab(ResourceEditor& asset_editor, string name, ResourceEditor::Tab type, VisualTileSet* asset_value, const std::function<void(bool)>& callback) {
    if (ImGui::BeginTabItem(name.c_str(), nullptr, type == asset_editor.external_tab_selection ? ImGuiTabItemFlags_SetSelected : 0)) {
        if (ImGui::Button("Reload##ResourceTab") || asset_editor.tab != type)
            asset_editor.switch_tab(type);
        
        bool changed = inspect(asset_value);

        if (ImGui::Button("Save##ResourceTab")) {
            save_resource(*asset_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##ResourceTab")) {
            *asset_value = load_resource<VisualTileSet>(asset_value->file_path, false, true);
            asset_editor.switch_tab(type);
        }
        if (callback)
            callback(changed);
        ImGui::EndTabItem();
    }
}


void ResourceEditor::window(bool* p_open) {
    ZoneScoped;
    
    if (ImGui::Begin("Resource Editor", p_open)) {
        if (ImGui::BeginTabBar("Resource Types", ImGuiTabBarFlags_FittingPolicyScroll)) {
            asset_tab(*this, ICON_FA_SITEMAP " Model", Tab_Model, &model_cpu, [this](bool changed) {
                if (model_cpu->skeleton != nullptr) {
                    PoseController* pose_controller = nullptr;
                    for (auto [entity, pc] : p_scene->registry.view<PoseController>().each())
                        pose_controller = &pc;

                    if (pose_controller == nullptr)
                        return;

                    static AnimationState play_state;
                    ImGui::EnumCombo("Play State", &play_state);
                    if (ImGui::Button("Play"))
                        pose_controller->set_state(play_state);
                    if (pose_controller->is_active()) {
                        ImGui::SameLine();                        
                        if (ImGui::Button("Stop"))
                            pose_controller->clear_state();
                    }
                }
            });
            asset_tab(*this, ICON_FA_TINT " Material", Tab_Material, &material_cpu, [this](bool changed) {
                if (changed)
                    get_gpu_asset_cache().materials.at(material_preview->material_id).update_from_cpu(material_cpu);
            });
//            asset_tab(*this, ICON_FA_USER " Lizard", Tab_Lizard, &lizard_prefab, [this](bool changed) {
//                if (ImGui::Checkbox("Force Target", &force_target)) {
//                    if (force_target) {
//                        for (auto [entity, caster] : p_scene->registry.view<Caster>().each()) {
//                            caster.taunt.set(0, v3i(3,0,0));
//                        }
//                    } else {
//                        for (auto [entity, caster] : p_scene->registry.view<Caster>().each()) {
//                            caster.taunt.reset(0);
//                        }
//                    }
//                }
//                if (!force_target) {
//                    if (ImGui::Button("Cast")) {
//                        for (auto [entity, caster] : p_scene->registry.view<Caster>().each()) {
//                            caster.spell->request_cast();
//                        }
//                    }
//                }
//                auto camera = p_scene->render_scene.viewport.camera;
//                auto line_mesh = generate_formatted_line(camera,
//                {
//                    {v3(0.f, 0.f, 0.f), palette::gray_4, 0.03f},
//                    {lizard_prefab.default_direction, palette::gray_4, 0.03f}
//                });
//
//
//                p_scene->render_scene.quick_mesh(line_mesh, true, true);
//            });
            asset_tab(*this, ICON_FA_CUBE " Tile", Tab_Tile, &tile_prefab, [this](bool changed) {
                Bitmask3D bitmask;
                bitmask.set(v3i(0));
                for (const v3i& solid : tile_prefab.solids)
                    bitmask.set(solid);
                auto mesh = generate_formatted_3d_bitmask(&p_scene->camera, bitmask);
                if (!mesh.vertices.empty())
                    p_scene->render_scene.quick_mesh(mesh, true, true);

                auto mesh2 = generate_formatted_dot(&p_scene->camera, {v3(tile_prefab.new_offset) + v3(0.5f, 0.5f, 0.5f), palette::spellbook_4, 0.03f});
                if (!mesh2.vertices.empty())
                    p_scene->render_scene.quick_mesh(mesh2, true, true);
            });
            asset_tab(*this, ICON_FA_APPLE " Enemy", Tab_Enemy, &enemy_prefab);
            asset_tab(*this, ICON_FA_SIGN_OUT "Spawner", Tab_Spawner, &spawner_prefab);
            asset_tab(*this, ICON_FA_SIGN_IN " Consumer", Tab_Consumer, &consumer_prefab);
            asset_tab(*this, ICON_FA_SNOWFLAKE_O " Emitter", Tab_Emitter, &emitter_cpu, [this](bool changed) {
                if (changed)
                    emitter_gpu->update_from_cpu(emitter_cpu, Input::time);
            });
            asset_tab(*this, ICON_FA_CUBES " Tile Set", Tab_TileSet, &tile_set);
            asset_tab(*this, ICON_FA_CIRCLE " Drop", Tab_Drop, &bead_prefab);
            asset_tab(*this, ICON_FA_MAP " Map", Tab_Map, &map_prefab);

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    external_tab_selection = Tab_None;
}


}
