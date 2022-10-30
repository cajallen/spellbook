#include "asset_editor.hpp"

#include <tracy/Tracy.hpp>

#include "asset_browser.hpp"
#include "game.hpp"

#include "lib_ext/imgui_extra.hpp"

#include "input.hpp"

#include "game/components.hpp"
#include "lib_ext/icons/font_awesome4.h"

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
                tab = Tab_Tower;
                PathSelect("File", &tower_prefab.file_path, "resources", possible_tower, "DND_TOWER");
                PathSelect("Model", &model_cpu.file_path, "resources", possible_model, "DND_MODEL");
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
            
            if (ImGui::BeginTabItem("Material")) {
                tab = Tab_Material;
                PathSelect("File", &material_cpu.file_path, "resources", possible_material, "DND_MATERIAL");

                ImGui::ColorEdit4("color_tint", material_cpu.color_tint.data, ImGuiColorEditFlags_DisplayHSV);
                ImGui::ColorEdit4("emissive_tint", material_cpu.emissive_tint.data, ImGuiColorEditFlags_DisplayHSV);
                ImGui::DragFloat("roughness_factor", &material_cpu.roughness_factor, 0.01f);
                ImGui::DragFloat("metallic_factor", &material_cpu.metallic_factor, 0.01f);
                ImGui::DragFloat("normal_factor", &material_cpu.normal_factor, 0.01f);

                PathSelect("color_asset_path", &material_cpu.color_asset_path, "resources", possible_texture, "DND_TEXTURE");
                PathSelect("orm_asset_path", &material_cpu.color_asset_path, "resources", possible_texture, "DND_TEXTURE");
                PathSelect("normal_asset_path", &material_cpu.color_asset_path, "resources", possible_texture, "DND_TEXTURE");
                PathSelect("emissive_asset_path", &material_cpu.color_asset_path, "resources", possible_texture, "DND_TEXTURE");

                EnumCombo("cull_mode", &material_cpu.cull_mode);
                
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
                PathSelect("File", &model_cpu.file_path, "resources", possible_model, "DND_MODEL");

                std::function<void(id_ptr<ModelCPU::Node>)> traverse;
                traverse = [&traverse, this](id_ptr<ModelCPU::Node> node) {
                    string id_str = fmt_("{}###{}", node->name, node.value);
                    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                    if (ImGui::TreeNode(id_str.c_str())) {
                        ImGui::InputText("Name", &node->name);
                        PathSelect("mesh_asset_path", &node->mesh_asset_path, "resources", possible_texture, "DND_TEXTURE");
                        PathSelect("material_asset_path", &node->material_asset_path, "resources", possible_material, "DND_MATERIAL");
                        DragMat4("Transform", &node->transform, 0.02f, "%.2f");
                        for (auto child : node->children) {
                            traverse(child);
                        }
                        if (ImGui::Button(ICON_FA_PLUS_CIRCLE, {-ImGui::GetStyle().FramePadding.x, 0})) {
                            model_cpu.nodes.insert_back(id_ptr<ModelCPU::Node>::emplace());
                            
                            model_cpu.nodes.last()->parent = node;
                            node->children.emplace_back(model_cpu.nodes.last());
                        }
                        ImGui::Separator();
                        ImGui::TreePop();
                    }
                };

                if (model_cpu.root_node.valid()) {
                    traverse(model_cpu.root_node);
                }
                
                if (ImGui::Button("Save")) {
                    save_model(model_cpu);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    model_cpu = load_model(model_cpu.file_path);
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
