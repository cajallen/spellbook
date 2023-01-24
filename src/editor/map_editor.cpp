#include "map_editor.hpp"

#include <tracy/Tracy.hpp>

#include "game_scene.hpp"
#include "widget_system.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/file.hpp"
#include "game/game.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"
#include "game/input.hpp"
#include "editor/asset_browser.hpp"
#include "extension/fmt.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

ADD_EDITOR_SCENE(MapEditor);

template <typename T>
bool show_buttons(const string& name, MapEditor& map_editor, vector<Button<T>>& buttons, u32* selected) {
    static umap<string, Button<T>> add_map;
    static umap<string, u32> edit_map;

    bool ret = false;
    
    ImGui::PushID(name.c_str());
    
    ImGui::Text("%s", name.c_str());

    if (!add_map.contains(name)) add_map[name] = {};

    if (ImGui::Button("Add")) {
        ImGui::OpenPopup("Add Button");
    }
    
    float  window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    ImVec2 button_size   = ImVec2(100, 30);
    for (u32 i = 0; i < buttons.size(); i++) {
        Color normal_color  = buttons[i].color;
        Color hovered_color = mix(normal_color, palette::white, 0.2f);
        Color pressed_color = mix(normal_color, palette::white, 0.1f);

        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) normal_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) hovered_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) pressed_color);
        if (ImGui::Button(buttons[i].text.c_str(), button_size)) {
            if (*selected == i)
                *selected = -1;
            else {
                map_editor.unselect_buttons();
                *selected = i;
            }
            ret = true;
        }
        bool open_popup = false;
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::Selectable("Edit")) {
                open_popup = true;
            }
            if (ImGui::Selectable("Delete")) {
                buttons.remove_index(i);
                i--;
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(3);
        
        if (open_popup) {
            ImGui::OpenPopup("Edit Button");
            edit_map[name] = i;
        }

        if (*selected == i) {
            v2 min = v2(ImGui::GetItemRectMin()) - v2(1);
            v2 max = v2(ImGui::GetItemRectMax()) + v2(1);

            ImGui::GetForegroundDrawList()->AddRect((ImVec2) min, (ImVec2) max, (u32) palette::white, 0.0f, 0, 2.0f);
        }
        float last_button_x2 = ImGui::GetItemRectMax().x;
        float next_button_x2 = last_button_x2 + ImGui::GetStyle().ItemSpacing.x + button_size.x;
        // Expected position if next button was on same line
        if (i + 1 < buttons.size() && next_button_x2 < window_visible_x2)
            ImGui::SameLine();
    }

    bool add_open = true;
    if (ImGui::BeginPopupModal("Add Button", &add_open)) {
        ImGui::InputText("Text", &add_map[name].text);
        ImGui::ColorEdit3("Color", add_map[name].color.data);
        ImGui::PathSelect("Path", &add_map[name].item_path, "resources", from_typeinfo(typeid(T)), true);
        
        if (ImGui::Button("Add")) {
            fs::path as_path = add_map[name].item_path;
            if (fs::is_directory(as_path)) {
                for (auto& dir_entry : fs::directory_iterator(as_path)) {
                    if (path_filter(from_typeinfo(typeid(T)))(dir_entry)) {
                        buttons.emplace_back(
                            dir_entry.path().stem().string(),
                            add_map[name].color,
                            dir_entry.path().string()
                        );
                    } else {
                        log_warning("Directory contents not added as button as it does not match types");
                    }
                }
            } else {
                buttons.emplace_back(std::move(add_map[name]));
            }
            add_map[name] = {};
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    bool edit_open;
    if (ImGui::BeginPopupModal("Edit Button", &edit_open)) {
        ImGui::InputText("Text", &buttons[edit_map[name]].text);
        ImGui::ColorEdit3("Color", buttons[edit_map[name]].color.data);
        ImGui::PathSelect("Path", &buttons[edit_map[name]].item_path, "resources", from_typeinfo(typeid(T)), true);

        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    return ret;
}

bool map_editor_key(KeyCallbackArgs args) {
    MapEditor& map_editor = *((MapEditor*) args.data);
    Viewport& viewport = map_editor.p_scene->render_scene.viewport;
    if (!viewport.hovered && !viewport.focused)
        return false;

    if (args.key == GLFW_KEY_E && args.action == GLFW_PRESS) {
        if (map_editor.eraser_selected) {
            map_editor.unselect_buttons();
        }
        else {
            map_editor.unselect_buttons();
            map_editor.eraser_selected = true;
        }
        return true;
    }
    return false;
}

bool map_editor_click_painting(ClickCallbackArgs args) {
    MapEditor& map_editor = *((MapEditor*) args.data);
    if (args.action == GLFW_PRESS && args.button == GLFW_MOUSE_BUTTON_LEFT) {
        if (map_editor.selected_consumer != ~0u ||
            map_editor.selected_lizard != ~0u ||
            map_editor.selected_tile != ~0u ||
            map_editor.selected_spawner != ~0u ||
            map_editor.eraser_selected) {
            map_editor.painting = true;
            return true;
        }
    }
    else if (args.action == GLFW_RELEASE && args.button == GLFW_MOUSE_BUTTON_LEFT) {
        map_editor.painting = false;
    }
    return false;
}

bool map_editor_click_dragging(ClickCallbackArgs args) {
    MapEditor& map_editor = *((MapEditor*) args.data);
    if (args.action == GLFW_PRESS) {
        map_editor.p_scene->render_scene.query = v2i(Input::mouse_pos) - map_editor.p_scene->render_scene.viewport.start;
        return true;
    }
    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        map_editor.p_scene->registry.clear<Dragging>();
        map_editor.p_scene->selected_entity = entt::null;
    }
    return false;
}

bool map_editor_scroll(ScrollCallbackArgs args) {
    MapEditor& map_editor = *((MapEditor*) args.data);
    Viewport& viewport = map_editor.p_scene->render_scene.viewport;
    
    if (!viewport.hovered && !viewport.focused)
        return false;
    
    if (Input::shift) {
        map_editor.y_level += (s32) args.yoffset;
        return true;
    }

    if (Input::alt) {
        map_editor.rotation = (map_editor.rotation + 4 + (s32) args.yoffset) % 4;
        return true;
    }
    return false;
}

void MapEditor::setup() {
    ZoneScoped;
    p_scene = new Scene();
    p_scene->setup("Map Edit Scene");
    p_scene->edit_mode = true;
    
    fs::path map_file = fs::path(game.user_folder) / ("map_editor" + extension(FileType_General));

    if (!fs::exists(map_file))
        return;
    
    json j = parse_file(map_file.string());
    FROM_JSON_MEMBER(lizard_buttons);
    FROM_JSON_MEMBER(tile_buttons);
    FROM_JSON_MEMBER(spawner_buttons);
    FROM_JSON_MEMBER(consumer_buttons);
    FROM_JSON_MEMBER(map_prefab.file_path);

    Input::add_callback(InputCallbackInfo{map_editor_key, 60, "map_editor", this});
    Input::add_callback(InputCallbackInfo{map_editor_scroll, 10, "map_editor", this});
    Input::add_callback(InputCallbackInfo{map_editor_click_painting, 20, "map_editor", this});
    Input::add_callback(InputCallbackInfo{map_editor_click_dragging, 60, "map_editor", this});
}


void MapEditor::update() {
    ZoneScoped;
    
    if (p_scene->pause)
        return;
    
    Viewport& viewport = p_scene->render_scene.viewport;
    if (viewport.hovered) {
        v3  intersect = math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, y_level);
        v3i top_drawn_cell      = (v3i) math::floor(intersect) - v3i(0,0,1);
        v3i bot_drawn_cell      = (v3i) math::floor(intersect);
        draw_preview(top_drawn_cell);

        // we should check if this mouse input is ours in the callback)
        if (painting) {
            if (eraser_selected) {
                vector<entt::entity> targets = p_scene->get_any(top_drawn_cell);
                p_scene->registry.destroy(targets.begin(), targets.end());
                map_prefab.tiles.erase(top_drawn_cell);
                map_prefab.lizards.erase(top_drawn_cell);
                map_prefab.consumers.erase(top_drawn_cell);
                map_prefab.spawners.erase(top_drawn_cell);
                map_prefab.solid_tiles.erase(top_drawn_cell);
            }
            else if (selected_tile != -1) {
                instance_and_write_prefab(load_asset<TilePrefab>(tile_buttons[selected_tile].item_path), top_drawn_cell);
                map_prefab.solid_tiles.insert(top_drawn_cell);
                build_visuals();
            }
            else if (selected_lizard != -1) {
                instance_and_write_prefab(load_asset<LizardPrefab>(lizard_buttons[selected_lizard].item_path), bot_drawn_cell);
            }
            else if (selected_spawner != -1) {
                instance_and_write_prefab(load_asset<SpawnerPrefab>(spawner_buttons[selected_spawner].item_path), bot_drawn_cell);
            }
            else if (selected_consumer != -1) {
                instance_and_write_prefab(load_asset<ConsumerPrefab>(consumer_buttons[selected_consumer].item_path), bot_drawn_cell);
            }
        }
    }
    
    p_scene->update();

    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->registry.clear<Dragging>();
        p_scene->selected_entity = entt::null;
    }
}

void MapEditor::window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Map Editor", p_open)) {
        if (ImGui::Button("Play")) {
            ADD_EDITOR_SCENE(GameScene);
            auto game_scene = (GameScene*) EditorScenes::values().back();
            game_scene->setup(map_prefab);
            p_scene->pause = true;
        }

        ImGui::PathSelect("VTS", &vts_path, "resources/visual_tile_sets", FileType_VisualTileSet);
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            visual_tileset = convert_to_entry_pool(load_asset<VisualTileSet>(vts_path));
            build_visuals();
        }
        
        
        ImGui::PathSelect("Map Prefab Path", &map_prefab.file_path, "resources/maps", FileType_Map, true);
        if (ImGui::Button("Load")) {
            p_scene->cleanup();
            delete p_scene;
            
            map_prefab = load_asset<MapPrefab>(map_prefab.file_path);
            p_scene = instance_map(map_prefab, "Map Edit Scene");
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            save_asset(map_prefab);
        }

        show_buttons("Lizards", *this, lizard_buttons, &selected_lizard);
        ImGui::Separator();
        show_buttons("Tiles", *this, tile_buttons, &selected_tile);
        ImGui::Separator();
        show_buttons("Spawners", *this, spawner_buttons, &selected_spawner);
        ImGui::Separator();
        show_buttons("Consumers", *this, consumer_buttons, &selected_consumer);
    } else {
        unselect_buttons();
    }
    ImGui::End();
}

void MapEditor::unselect_buttons() {
    eraser_selected = false;
    selected_tile = -1;
    selected_lizard = -1;
    selected_spawner = -1;
    selected_consumer = -1;
}


void MapEditor::shutdown() {
    fs::path map_editor_file = fs::path(game.user_folder) / ("map_editor" + extension(FileType_General));
    fs::create_directories(map_editor_file.parent_path());
    
    auto j = json();
    TO_JSON_MEMBER(lizard_buttons);
    TO_JSON_MEMBER(tile_buttons);
    TO_JSON_MEMBER(spawner_buttons);
    TO_JSON_MEMBER(consumer_buttons);

    file_dump(j, map_editor_file.string());

    Input::remove_callback<KeyCallback>("map_editor");
    Input::remove_callback<ClickCallback>("map_editor");
}

void MapEditor::build_visuals() {
    for (entt::entity e : visual_map_entities) {
        p_scene->registry.destroy(e);
    }
    visual_map_entities.clear();
    auto visual_tiles = build_visual_tiles(map_prefab.solid_tiles, visual_tileset);
    for (auto& [pos, tile_entry] : visual_tiles) {
        auto entity = p_scene->registry.create();
        p_scene->registry.emplace<Name>(entity, fmt_("tile:({},{},{})",pos.x,pos.y,pos.z));
        visual_map_entities.push_back(entity);
                
        auto& model_comp = p_scene->registry.emplace<Model>(entity);
        model_comp.model_cpu = load_asset<ModelCPU>(tile_entry.model_path);
        model_comp.model_gpu = instance_model(p_scene->render_scene, model_comp.model_cpu);
                
        p_scene->registry.emplace<ModelTransform>(entity,
            v3(pos) + v3(1.0f),
            euler{tile_entry.rotation.yaw * math::PI * 0.5f},
            tile_entry.rotation.flip ? v3(-1.0f, 1.0f, 1.0f) : v3(1.0f, 1.0f, 1.0f)
        );
    }
}


void MapEditor::draw_preview(v3i cell) {
    constexpr float line_width = 0.03f;
    auto camera = p_scene->render_scene.viewport.camera;
    if (eraser_selected) {
        auto line_mesh = generate_formatted_line(camera,
        {
            {(v3) cell + v3(0.f, 0.f, 0.f), palette::gray_4, line_width},
            {(v3) cell + v3(1.f, 1.f, 0.f), palette::gray_4, line_width},
            {(v3) cell + v3(0.f, 1.f, 0.f), palette::gray_4, line_width},
            {(v3) cell + v3(1.f, 0.f, 0.f), palette::gray_4, line_width},
            {(v3) cell + v3(1.f, 0.f, 1.f), palette::gray_4, line_width},
            {(v3) cell + v3(0.f, 1.f, 1.f), palette::gray_4, line_width},
            {(v3) cell + v3(1.f, 1.f, 1.f), palette::gray_4, line_width},
            {(v3) cell + v3(0.f, 0.f, 1.f), palette::gray_4, line_width},
            {(v3) cell + v3(0.f, 0.f, 0.f), palette::gray_4, line_width},
            {(v3) cell + v3(0.f, 1.f, 0.f), palette::gray_4, line_width},
            {(v3) cell + v3(0.f, 1.f, 1.f), palette::gray_4, line_width},
            {(v3) cell + v3(0.f, 0.f, 1.f), palette::gray_4, line_width},
            {(v3) cell + v3(1.f, 0.f, 1.f), palette::gray_4, line_width},
            {(v3) cell + v3(1.f, 1.f, 1.f), palette::gray_4, line_width},
            {(v3) cell + v3(1.f, 1.f, 0.f), palette::gray_4, line_width},
            {(v3) cell + v3(1.f, 0.f, 0.f), palette::gray_4, line_width}
        });

        p_scene->render_scene.quick_mesh(line_mesh, true, true);
    } else if (selected_tile != -1) {
        auto line_mesh = generate_formatted_line(camera,
        {
            {(v3) cell + v3(0.f, 0.f, 1.05f), palette::white, line_width},
            {(v3) cell + v3(1.f, 0.f, 1.05f), palette::white, line_width},
            {(v3) cell + v3(1.f, 1.f, 1.05f), palette::white, line_width},
            {(v3) cell + v3(0.f, 1.f, 1.05f), palette::white, line_width},
            {(v3) cell + v3(0.f, 0.f, 1.05f), palette::white, line_width},
            {(v3) cell + v3(0.f, 0.f, 1.05f), palette::white, line_width}
        });

        p_scene->render_scene.quick_mesh(line_mesh, true, true);
    } else if (selected_lizard != -1) {
        vector<FormattedVertex> vertices;
        for (int i = 0; i <= 48; i++) {
            f32 angle  = i * math::TAU / 48.0f;
            v3  center = (v3) cell + v3(0.5f, 0.5f, 1.5f);
            vertices.emplace_back(center + 0.5f * v3(math::cos(angle), math::sin(angle), 0.04f), palette::white, line_width);
        }

        auto line_mesh = generate_formatted_line(camera, std::move(vertices));
        p_scene->render_scene.quick_mesh(line_mesh, true, true);
    }
}


}
