#include "map_editor.hpp"

#include <tracy/Tracy.hpp>

#include "lib_ext/fmt_geometry.hpp"

#include "scene.hpp"
#include "game.hpp"
#include "components.hpp"
#include "input.hpp"
#include "file.hpp"

#include "asset_browser.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {


bool map_editor_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods, void* data) {
    MapEditor& map_editor = *((MapEditor*) data);
    Viewport& viewport = map_editor.p_scene->render_scene.viewport;
    if (!viewport.hovered && !viewport.focused)
        return false;

    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
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

void MapEditor::setup() {
    ZoneScoped;
    p_scene = new Scene();
    p_scene->setup("Map Edit Scene");
    
    fs::path map_file = fs::path(game.user_folder) / ("map_editor" + extension(FileType_General));

    if (!fs::exists(map_file))
        return;
    
    json j = parse_file(map_file.string());
    FROM_JSON_MEMBER(tower_buttons);
    FROM_JSON_MEMBER(tile_buttons);
    FROM_JSON_MEMBER(spawner_buttons);
    FROM_JSON_MEMBER(consumer_buttons);
    FROM_JSON_MEMBER(map_prefab.file_path);

    Input::key_callback_stack.insert(0, {map_editor_key_callback, "map_editor", this});
}


void MapEditor::update() {
    ZoneScoped;
    Viewport& viewport = p_scene->render_scene.viewport;

    if (!viewport.hovered)
        return;

    v3  intersect = math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);
    v3i cell      = (v3i) math::floor(intersect) - v3i(0,0,1);
    draw_preview(cell);

    // we should check if this mouse input is ours in the callback)
    bool input_used = false;
    if (Input::mouse_press_at[GLFW_MOUSE_BUTTON_LEFT] > 0.0f) {
        if (eraser_selected) {
            vector<entt::entity> targets = p_scene->get_any(cell);
            p_scene->registry.destroy(targets.begin(), targets.end());
            map_prefab.tiles.erase(cell);
            map_prefab.towers.erase(cell);
            map_prefab.consumers.erase(cell);
            map_prefab.spawners.erase(cell);
            input_used = true;
        }
        else if (selected_tile != -1) {
            instance_and_write_prefab(load_tile(tile_buttons[selected_tile].item_path), cell);
            input_used = true;
        }
        else if (selected_tower != -1) {
            v3i new_pos = cell + v3i(0,0,1);
            instance_and_write_prefab(load_tower(tower_buttons[selected_tower].item_path), new_pos);
            input_used = true;
        }
        else if (selected_spawner != -1) {
            v3i new_pos = cell + v3i(0,0,1);
            instance_and_write_prefab(load_spawner(spawner_buttons[selected_spawner].item_path), new_pos);
            input_used = true;
        }
        else if (selected_consumer != -1) {
            v3i new_pos = cell + v3i(0,0,1);
            instance_and_write_prefab(load_consumer(consumer_buttons[selected_consumer].item_path), new_pos);
            input_used = true;
        }
    }
    
    if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT] && !input_used) {
        p_scene->render_scene.query = v2i(Input::mouse_pos) - p_scene->render_scene.viewport.start;
        input_used = true;
    }

    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->registry.clear<Dragging>();
        p_scene->selected_entity = entt::null;
    }

}

void MapEditor::window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Map Editor", p_open)) {
        ImGui::PathSelect("Map Prefab Path", &map_prefab.file_path, "resources/maps", FileType_Map, true);
        if (ImGui::Button("Load")) {
            p_scene->cleanup();
            delete p_scene;
            
            map_prefab = load_map(map_prefab.file_path);
            p_scene = instance_map(map_prefab, "Map Edit Scene");
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            save_map(map_prefab);
        }

        show_buttons("Towers", *this, tower_buttons, &selected_tower);
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
    selected_tower = -1;
    selected_spawner = -1;
    selected_consumer = -1;
}


void MapEditor::shutdown() {
    fs::path map_editor_file = fs::path(game.user_folder) / ("map_editor" + extension(FileType_General));
    fs::create_directories(map_editor_file.parent_path());
    
    auto j = json();
    TO_JSON_MEMBER(tower_buttons);
    TO_JSON_MEMBER(tile_buttons);
    TO_JSON_MEMBER(spawner_buttons);
    TO_JSON_MEMBER(consumer_buttons);

    file_dump(j, map_editor_file.string());

    Input::remove_key_callback("map_editor");
}

void MapEditor::draw_preview(v3i cell) {
    if (eraser_selected) {
        auto line_mesh = generate_formatted_line(p_scene->render_scene.viewport.camera,
        {
            {(v3) cell + v3(0.f, 0.f, 0.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(1.f, 1.f, 0.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(0.f, 1.f, 0.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(1.f, 0.f, 0.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(1.f, 0.f, 1.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(0.f, 1.f, 1.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(1.f, 1.f, 1.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(0.f, 0.f, 1.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(0.f, 0.f, 0.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(0.f, 1.f, 0.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(0.f, 1.f, 1.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(0.f, 0.f, 1.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(1.f, 0.f, 1.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(1.f, 1.f, 1.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(1.f, 1.f, 0.f), palette::gray_4, 0.03f},
            {(v3) cell + v3(1.f, 0.f, 0.f), palette::gray_4, 0.03f}
        });

        p_scene->render_scene.quick_mesh(line_mesh);
    } else if (selected_tile != -1) {
        auto line_mesh = generate_formatted_line(p_scene->render_scene.viewport.camera,
        {
            {(v3) cell + v3(0.f, 0.f, 1.05f), palette::white, 0.03f},
            {(v3) cell + v3(1.f, 0.f, 1.05f), palette::white, 0.03f},
            {(v3) cell + v3(1.f, 1.f, 1.05f), palette::white, 0.03f},
            {(v3) cell + v3(0.f, 1.f, 1.05f), palette::white, 0.03f},
            {(v3) cell + v3(0.f, 0.f, 1.05f), palette::white, 0.03f},
            {(v3) cell + v3(0.f, 0.f, 1.05f), palette::white, 0.03f}
        });

        p_scene->render_scene.quick_mesh(line_mesh);
    } else if (selected_tower != -1) {
        vector<FormattedVertex> vertices;
        for (int i = 0; i <= 24; i++) {
            f32 angle  = i * math::TAU / 24.0f;
            v3  center = (v3) cell + v3(0.5f, 0.5f, 1.5f);
            vertices.emplace_back(center + 0.5f * v3(math::cos(angle), math::sin(angle), 0.05f), palette::white, 0.03f);
        }

        auto line_mesh = generate_formatted_line(p_scene->render_scene.viewport.camera, std::move(vertices));
        p_scene->render_scene.quick_mesh(line_mesh);
    }
}


}
