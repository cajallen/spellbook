
#include "map_editor.hpp"

#include <tracy/Tracy.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/math/matrix_math.hpp"
#include "editor/console.hpp"
#include "editor/pose_widget.hpp"
#include "editor/game_scene.hpp"
#include "editor/widget_system.hpp"
#include "editor/asset_browser.hpp"
#include "renderer/draw_functions.hpp"
#include "game/game.hpp"
#include "game/scene.hpp"
#include "game/input.hpp"
#include "game/game_file.hpp"
#include "game/game_path.hpp"
#include "game/tile_set_generator.hpp"
#include "game/entities/components.hpp"
#include "game/entities/tile.hpp"
#include "game/entities/spawner.hpp"
#include "game/entities/consumer.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/lizard.hpp"

#include "general/astar.hpp"

namespace spellbook {

ADD_EDITOR_SCENE(MapEditor);

template <typename T>
bool show_buttons(const string& name, MapEditor& map_editor, vector<Button<T>>& buttons, uint32* selected) {
    static umap<string, Button<T>> add_map;
    static umap<string, uint32> edit_map;

    bool ret = false;
    
    ImGui::PushID(name.c_str());
    
    ImGui::Text("%s", name.c_str());

    if (!add_map.contains(name)) add_map[name] = {};

    if (ImGui::Button("Add")) {
        ImGui::OpenPopup("Add Button");
    }
    
    float  window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    ImVec2 button_size   = ImVec2(100, 30);
    for (uint32 i = 0; i < buttons.size(); i++) {
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

            ImGui::GetForegroundDrawList()->AddRect((ImVec2) min, (ImVec2) max, (uint32) palette::white, 0.0f, 0, 2.0f);
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
        ImGui::PathSelect("Path", &add_map[name].item_path, from_typeinfo(typeid(T)), 1);
        
        if (ImGui::Button("Add")) {
            fs::path as_path = add_map[name].item_path.abs_path();
            if (fs::is_directory(as_path)) {
                for (auto& dir_entry : fs::directory_iterator(as_path)) {
                    if (path_filter(from_typeinfo(typeid(T)))(dir_entry)) {
                        buttons.emplace_back(
                            dir_entry.path().stem().string(),
                            add_map[name].color,
                            FilePath(dir_entry.path())
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
        ImGui::PathSelect("Path", &buttons[edit_map[name]].item_path, from_typeinfo(typeid(T)), 1);

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
    Viewport& viewport = map_editor.p_scene->render_scene.viewport;
    if (!viewport.hovered && !viewport.focused)
        return false;
    
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
        map_editor.last_paint = true;
    }
    return false;
}

bool map_editor_scroll(ScrollCallbackArgs args) {
    MapEditor& map_editor = *((MapEditor*) args.data);
    Viewport& viewport = map_editor.p_scene->render_scene.viewport;
    
    if (!viewport.hovered && !viewport.focused)
        return false;
    
    if (Input::shift) {
        map_editor.z_level += (int32) args.yoffset;
        return true;
    }

    if (Input::alt) {
        map_editor.rotation = (map_editor.rotation + 4 + (int32) args.yoffset) % 4;
        return true;
    }
    return false;
}

void MapEditor::setup() {
    ZoneScoped;
    // generate_example_set();

    FilePath map_editor_file = FilePath(string(game.user_folder) + "map_editor" + extension(FileType_General));
    if (!fs::exists(map_editor_file.abs_path()))
        return;
    
    json j = parse_file(map_editor_file.abs_string());
    FROM_JSON_MEMBER(lizard_buttons);
    FROM_JSON_MEMBER(tile_buttons);
    FROM_JSON_MEMBER(spawner_buttons);
    FROM_JSON_MEMBER(consumer_buttons);
    FROM_JSON_MEMBER(map_prefab.file_path);
    FROM_JSON_MEMBER(vts_path);
    
    if (vts_path.is_file())
        visual_tileset = convert_to_entry_pool(load_asset<VisualTileSet>(vts_path));
    if (map_prefab.file_path.is_file()) {
        map_prefab = load_asset<MapPrefab>(map_prefab.file_path);
        setup_scene(instance_map(map_prefab, "Map Edit Scene"), true);
    } else {
        setup_scene(new Scene(), false);
    }

    Input::add_callback(InputCallbackInfo{map_editor_key, 60, "map_editor", this});
    Input::add_callback(InputCallbackInfo{map_editor_scroll, 10, "map_editor", this});
    Input::add_callback(InputCallbackInfo{map_editor_click_painting, 20, "map_editor", this});
}

void MapEditor::setup_scene(Scene* scene, bool scene_setup) {
    p_scene = scene;
    if (!scene_setup)
        p_scene->setup("Map Edit Scene");
    p_scene->set_edit_mode(true);
    if (map_prefab.file_path.is_file() && vts_path.is_file())
        build_visuals(scene, nullptr);
}


void MapEditor::update() {
    ZoneScoped;

    if (p_scene->pause)
        return;

    Viewport& viewport = p_scene->render_scene.viewport;
    
    if (viewport.hovered) {
        v3i cell;
        bool auto_adjust = (selected_lizard != ~0u || selected_spawner != ~0u || selected_consumer != ~0u);
        if (selected_tile != ~0u)
            auto_adjust = load_asset<TilePrefab>(tile_buttons[selected_tile].item_path).type == TileType_Scenery;
        if (auto_adjust && p_scene->get_object_placement(cell)) {
            if (selected_tile != ~0u)
                cell.z--;
            z_level = cell.z;
        } else {
            if (eraser_selected || selected_tile != -1) {
                cell = math::floor_cast(math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, z_level));
                cell.z -= 1;
            }
            else
                cell = math::floor_cast(math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, z_level));
        }
        draw_preview(cell);

        // we should check if this mouse input is ours in the callback)
        if (painting) {
            if (eraser_selected) {
                vector<entt::entity> targets = p_scene->get_any(cell);
                p_scene->registry.destroy(targets.begin(), targets.end());
                map_prefab.tiles.erase(cell);
                map_prefab.lizards.erase(cell);
                map_prefab.consumers.erase(cell);
                map_prefab.spawners.erase(cell);
                map_prefab.solid_tiles.erase(cell);

                build_visuals(p_scene, &cell);
            }
            else if (selected_tile != -1) {
                TilePrefab tile_prefab = load_asset<TilePrefab>(tile_buttons[selected_tile].item_path);
                if (tile_prefab.type != TileType_Scenery || last_paint) {
                    instance_and_write_tile(tile_buttons[selected_tile].item_path, cell, rotation);
                    build_visuals(p_scene, &cell);
                }
            }
            else if (selected_spawner != -1) {
                instance_and_write_spawner(spawner_buttons[selected_spawner].item_path, cell);
            }
            else if (selected_consumer != -1 && last_paint) {
                instance_and_write_consumer(consumer_buttons[selected_consumer].item_path, cell);
            }
            else if (selected_lizard != -1) {
                instance_and_write_lizard(lizard_buttons[selected_lizard].item_path, cell);
            }
        }
    }
    
    p_scene->update();

    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->registry.clear<Dragging>();
        p_scene->selected_entity = entt::null;
    }

    if (last_paint) {
        painting = false;
        last_paint = false;
    }
}

void MapEditor::window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Map Editor", p_open)) {
        if (ImGui::Button("Play")) {
            p_scene->audio.play_sound("audio/page_flip.flac"_rp, {.global = true, .volume = 0.3f});

            console({.str="Playing map..."});
            
            ADD_EDITOR_SCENE(GameScene);
            auto game_scene = (GameScene*) EditorScenes::values().back();
            game_scene->setup(map_prefab);
            p_scene->pause = true;
            build_visuals(game_scene->p_scene, nullptr);
        }

        ImGui::PathSelect("VTS", &vts_path, FileType_VisualTileSet);
        ImGui::SameLine();
        if (ImGui::Button("Load##VTS")) {
            visual_tileset = convert_to_entry_pool(load_asset<VisualTileSet>(vts_path));
            build_visuals(p_scene, nullptr);
        }
        
        
        ImGui::PathSelect("Map Prefab Path", &map_prefab.file_path, FileType_Map, 1);
        ImGui::SameLine();
        if (ImGui::Button("Load##Map")) {
            p_scene->cleanup();
            delete p_scene;
            
            map_prefab = load_asset<MapPrefab>(map_prefab.file_path);
            setup_scene(instance_map(map_prefab, "Map Edit Scene"), true);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            save_asset(map_prefab);
        }

        show_buttons("Tiles", *this, tile_buttons, &selected_tile);
        ImGui::Separator();
        show_buttons("Spawners", *this, spawner_buttons, &selected_spawner);
        ImGui::Separator();
        show_buttons("Consumers", *this, consumer_buttons, &selected_consumer);
        ImGui::Separator();
        show_buttons("Lizards", *this, lizard_buttons, &selected_lizard);
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
    TO_JSON_MEMBER(map_prefab.file_path);
    TO_JSON_MEMBER(vts_path);

    file_dump(j, map_editor_file.string());

    Input::remove_callback<KeyCallback>("map_editor");
    Input::remove_callback<ClickCallback>("map_editor");
}

void MapEditor::instance_and_write_consumer(const FilePath& path, v3i pos) {
    entt::entity old_tile = p_scene->get_shrine(pos);
    if (old_tile != entt::null) {
        p_scene->registry.destroy(old_tile);
    }
        
    map_prefab.consumers[pos] = path;
    instance_prefab(p_scene, load_asset<ConsumerPrefab>(path), pos);
}

void MapEditor::instance_and_write_spawner(const FilePath& path, v3i pos) {
    entt::entity old_tile = p_scene->get_spawner(pos);
    if (old_tile != entt::null) {
        p_scene->registry.destroy(old_tile);
    }
        
    map_prefab.spawners[pos] = path;
    instance_prefab(p_scene, load_asset<SpawnerPrefab>(path), pos);
}

void MapEditor::instance_and_write_lizard(const FilePath& path, v3i pos) {
    entt::entity old_tile = p_scene->targeting->select_lizard(pos);
    if (old_tile != entt::null) {
        p_scene->registry.destroy(old_tile);
    }
        
    map_prefab.lizards[pos] = path;
    instance_prefab(p_scene, load_asset<LizardPrefab>(path), pos);
}

void MapEditor::instance_and_write_tile(const FilePath& path, v3i input_pos, uint32 rotation) {
    TilePrefab tile_prefab = load_asset<TilePrefab>(path);
    auto type = tile_prefab.type;
    v3i pos;
    switch (type) {
        case (TileType_Path):
            pos = input_pos;
            map_prefab.solid_tiles[pos] = 0b100;
        break;
        case (TileType_TowerSlot):
            pos = input_pos;
            map_prefab.solid_tiles[pos] = 0b010;
        break;
        case (TileType_Ramp):
            pos = input_pos;
            map_prefab.solid_tiles[pos] = 0b001;
        break;
        default:
            pos = input_pos + v3i(0,0,1);
            map_prefab.solid_tiles[pos] = 0b001;
    }
        
    entt::entity old_tile = p_scene->get_tile(pos);
    if (old_tile != entt::null) {
        p_scene->registry.destroy(old_tile);
    }

    map_prefab.tiles[pos] = {path, rotation};
    instance_prefab(p_scene, tile_prefab, pos, rotation);
}

void MapEditor::build_visuals(Scene* scene, v3i* tile) {
    auto visual_tiles = build_visual_tiles(map_prefab.solid_tiles, visual_tileset, tile);
    for (auto& [pos, tile_entry] : visual_tiles) {
        if (scene->visual_map_entities.contains(pos)) {
            scene->registry.destroy(scene->visual_map_entities[pos]);
            scene->visual_map_entities.erase(pos);
        }

        if (!tile_entry.model_path.is_file())
            continue;
        
        auto entity = scene->registry.create();
        scene->visual_map_entities[pos] = entity;

        StaticModel& model = scene->registry.emplace<StaticModel>(entity,
            instance_static_model(scene->render_scene, load_asset<ModelCPU>(tile_entry.model_path))
        );

        m44GPU tfm = m44GPU(
            math::translate(v3(pos) + v3(1.0f)) *
            math::rotation(quat(v3::Z, tile_entry.rotation.yaw * math::PI * 0.5f)) *
            math::scale(v3(tile_entry.rotation.flip_x ? -1.0f : 1.0f, 1.0f, tile_entry.rotation.flip_z ? -1.0f : 1.0f))
        );
        for (StaticRenderable* renderable : model.renderables) {
            renderable->transform = tfm;
        }
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
        TilePrefab tile_prefab = load_asset<TilePrefab>(tile_buttons[selected_tile].item_path);
        if (tile_prefab.type == TileType_Ramp) {
            v3 dir;
            v3 perp;
            switch (rotation) {
                case 0: {
                    dir = v3(0.5f, 0.0f, 0.5f);
                    perp = v3(0.0f, 0.5f, 0.0f);
                } break;
                case 1: {
                    dir = v3(0.0f, 0.5f, 0.5f);
                    perp = v3(0.5f, 0.0f, 0.0f);
                } break;
                case 2: {
                    dir = v3(-0.5f, 0.0f, 0.5f);
                    perp = v3(0.0f, 0.5f, 0.0f);
                } break;
                case 3: {
                    dir = v3(0.0f, -0.5f, 0.5f);
                    perp = v3(0.5f, 0.0f, 0.0f);
                } break;
            }
            auto ramp_line_mesh = generate_formatted_line(camera,
            {
                {(v3) cell + v3(0.5f) - dir - perp, palette::white, line_width},
                {(v3) cell + v3(0.5f) + dir - perp, palette::white, line_width},
                {(v3) cell + v3(0.5f) + dir + perp, palette::white, line_width},
                {(v3) cell + v3(0.5f) - dir + perp, palette::white, line_width},
                {(v3) cell + v3(0.5f) - dir - perp, palette::white, line_width}
            });
            p_scene->render_scene.quick_renderable(ramp_line_mesh, true, true);
        } else {
            Bitmask3D bitmask;
            bitmask.set(cell + v3i::Z);
            quat r = quat(v3::Z, math::PI * 0.5f * float(rotation));
            for (const v3i& offset : tile_prefab.solids) {
                v3 loc = v3(cell + v3i::Z) + math::rotate(r, v3(offset));
                bitmask.set(math::round_cast(loc));
            }
            auto mesh = generate_formatted_3d_bitmask(&p_scene->camera, bitmask);
            if (!mesh.vertices.empty())
                p_scene->render_scene.quick_mesh(mesh, true, true);
        }
    } else if (selected_lizard != -1 || selected_spawner != -1 || selected_consumer != -1) {
        vector<FormattedVertex> vertices;
        for (int i = 0; i <= 48; i++) {
            float angle  = i * math::TAU / 48.0f;
            v3  center = (v3) cell + v3(0.5f, 0.5f, 0.05f);
            vertices.emplace_back(center + 0.4f * v3(math::cos(angle), math::sin(angle), 0.04f), palette::white, line_width);
        }

        auto line_mesh = generate_formatted_line(camera, std::move(vertices));
        p_scene->render_scene.quick_mesh(line_mesh, true, true);
    }
}


}
