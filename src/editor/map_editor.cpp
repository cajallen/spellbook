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
#include "renderer/draw_functions.hpp"

namespace spellbook {

ADD_EDITOR_SCENE(MapEditor);

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
            }
            else if (selected_tile != -1) {
                instance_and_write_prefab(load_asset<TilePrefab>(tile_buttons[selected_tile].item_path), top_drawn_cell);
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
