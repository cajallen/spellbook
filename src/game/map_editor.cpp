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

bool map_editor_mb_callback(GLFWwindow* window, int button, int action, int mods, void* data) {
    return false;
}

bool map_editor_mp_callback(GLFWwindow* window, double xpos, double ypos, void* data) {
    return false;
}

void MapEditor::setup() {
    ZoneScoped;
    p_scene = new Scene();

    fs::path map_file = fs::path(game.user_folder) / ("map_editor" + extension(FileType_General));

    if (!fs::exists(map_file))
        return;
    
    json j = parse_file(map_file.string());
    FROM_JSON_MEMBER(tower_buttons);
    FROM_JSON_MEMBER(tile_buttons);
    FROM_JSON_MEMBER(spawner_buttons);
    FROM_JSON_MEMBER(consumer_buttons);
}


void MapEditor::update() {
    ZoneScoped;
    Viewport& viewport = p_scene->render_scene.viewport;

    if (!viewport.hovered)
        return;

    v3  intersect = math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);
    v3i cell      = (v3i) math::floor(intersect);
    if (selected_tile != -1) {
        auto line_mesh = generate_formatted_line(p_scene->render_scene.viewport.camera,
        {
            {(v3) cell + v3(0.f, 0.f, 0.05f), palette::white, 0.03f},
            {(v3) cell + v3(1.f, 0.f, 0.05f), palette::white, 0.03f},
            {(v3) cell + v3(1.f, 1.f, 0.05f), palette::white, 0.03f},
            {(v3) cell + v3(0.f, 1.f, 0.05f), palette::white, 0.03f},
            {(v3) cell + v3(0.f, 0.f, 0.05f), palette::white, 0.03f},
            {(v3) cell + v3(0.f, 0.f, 0.05f), palette::white, 0.03f}});

        p_scene->render_scene.quick_mesh(line_mesh);
    } else if (selected_tower != -1) {
        vector<FormattedVertex> vertices;
        for (int i = 0; i <= 24; i++) {
            f32 angle  = i * math::TAU / 24.0f;
            v3  center = (v3) cell + v3(0.5f, 0.5f, 0.0f);
            vertices.emplace_back(center + 0.5f * v3(math::cos(angle), math::sin(angle), 0.05f), palette::white, 0.03f);
        }

        auto line_mesh = generate_formatted_line(p_scene->render_scene.viewport.camera, std::move(vertices));
        p_scene->render_scene.quick_mesh(line_mesh);
    }

    // we should check if this mouse input is ours in the callback)
    bool input_used = false;
    if (Input::mouse_press_at[GLFW_MOUSE_BUTTON_LEFT] > 0.0f) {
        if (selected_tile != -1) {
            v3i new_pos = cell - v3i(0,0,1);
            instance_and_write_prefab(load_tile(tile_buttons[selected_tile].item_path), new_pos);
            input_used = true;
        }
        else if (selected_tower != -1) {
            instance_and_write_prefab(load_tower(tower_buttons[selected_tower].item_path), cell);
            input_used = true;
        }
        else if (selected_spawner != -1) {
            instance_and_write_prefab(load_spawner(spawner_buttons[selected_spawner].item_path), cell);
            input_used = true;
        }
        else if (selected_consumer != -1) {
            instance_and_write_prefab(load_consumer(consumer_buttons[selected_consumer].item_path), cell);
            input_used = true;
        }
    }
    
    if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT] && !input_used) {
        p_scene->render_scene.query = v2i(Input::mouse_pos) - p_scene->render_scene.viewport.start;
        input_used = true;
    }

    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT] && p_scene->registry.valid(p_scene->selected_entity)) {
        p_scene->registry.remove<Dragging>(p_scene->selected_entity);
        p_scene->selected_entity = entt::null;
    }

}

void MapEditor::window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Map Editor", p_open)) {
        ImGuiStyle& style     = ImGui::GetStyle();
        Viewport&   viewport  = p_scene->render_scene.viewport;
        v3          intersect = math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);
        ImGui::Text(fmt_("hovered_cell: {}", intersect).c_str());

        if (show_buttons("Towers", tower_buttons, &selected_tower)) {
            selected_tile = -1;
            selected_spawner = -1;
            selected_consumer = -1;
        }
        ImGui::Separator();
        if (show_buttons("Tiles", tile_buttons, &selected_tile)) {
            selected_tower = -1;
            selected_spawner = -1;
            selected_consumer = -1;
        }
        ImGui::Separator();
        if (show_buttons("Spawners", spawner_buttons, &selected_spawner)) {
            selected_tile = -1;
            selected_tower = -1;
            selected_consumer = -1;
        }
        ImGui::Separator();
        if (show_buttons("Consumers", consumer_buttons, &selected_consumer)) {
            selected_tile = -1;
            selected_tower = -1;
            selected_spawner = -1;
        }
    } else {
        selected_tile = -1;
        selected_tower = -1;
        selected_spawner = -1;
        selected_consumer = -1;
    }
    ImGui::End();
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
}

}
