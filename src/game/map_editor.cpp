#include "map_editor.hpp"

#include <tracy/Tracy.hpp>

#include "lib_ext/fmt_geometry.hpp"

#include "scene.hpp"
#include "game.hpp"
#include "components.hpp"
#include "input.hpp"

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
    setup(new Scene());
}

void MapEditor::setup(Scene* init_scene) {
    p_scene = init_scene;
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
        Renderable brush_preview;
        brush_preview.mesh_asset_path = game.renderer.upload_mesh(line_mesh);
        brush_preview.material_asset_path = "default";
        brush_preview.frame_allocated = true;

        p_scene->render_scene.add_renderable(brush_preview);
    } else if (selected_tower != -1) {
        vector<FormattedVertex> vertices;
        for (int i = 0; i <= 24; i++) {
            f32 angle  = i * math::TAU / 24.0f;
            v3  center = (v3) cell + v3(0.5f, 0.5f, 0.0f);
            vertices.emplace_back(center + 0.5f * v3(math::cos(angle), math::sin(angle), 0.05f), palette::white, 0.03f);
        }

        auto line_mesh = generate_formatted_line(p_scene->render_scene.viewport.camera, std::move(vertices));
        Renderable tower_preview;
        tower_preview.mesh_asset_path = game.renderer.upload_mesh(line_mesh);
        tower_preview.material_asset_path = "default";
        tower_preview.frame_allocated = true;

        p_scene->render_scene.add_renderable(tower_preview);
    }

    // we should check if this mouse input is ours in the callback)
    bool input_used = false;
    if (Input::mouse_press_at[GLFW_MOUSE_BUTTON_LEFT] > 0.0f) {
        if (selected_tile != -1) {
            auto view  = p_scene->registry.view<GridSlot, LogicTransform, Model>();
            for (auto [entity, obj, logic_pos, model] : view.each()) {
                if (math::length(logic_pos.position - v3(cell)) < 0.1f) {
                    deinstance_model(p_scene->render_scene, model.model_gpu);
                    p_scene->registry.destroy(entity);
                    break;
                }
            }

            instance_prefab(p_scene, tiles[selected_tile].item, cell);
            input_used = true;
        }
        else if (selected_tower != -1) {
            auto view = p_scene->registry.view<LogicTransform, Model>();
            for (auto [entity, logic_pos, model] : view.each()) {
                if (!p_scene->registry.any_of<Pyro, Roller>(entity))
                    continue;
                if (math::length(logic_pos.position - v3(cell)) < 0.1f) {
                    deinstance_model(p_scene->render_scene, model.model_gpu);
                    p_scene->registry.destroy(entity);
                    break;
                }
            }

            instance_prefab(p_scene, towers[selected_tower].item, cell);
            input_used = true;
        }
        else if (selected_enemy != -1) {
            auto view = p_scene->registry.view<Traveler, LogicTransform, Model>();
            for (auto [entity, traveler, logic_pos, model] : view.each()) {
                if (math::length(logic_pos.position - v3(cell)) < 0.1f) {
                    deinstance_model(p_scene->render_scene, model.model_gpu);
                    p_scene->registry.destroy(entity);
                    break;
                }
            }

            instance_prefab(p_scene, towers[selected_tower].item, cell);
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

        if (show_buttons("Towers", towers, &selected_tower)) {
            selected_tile = -1;
            selected_enemy = -1;
        }
        ImGui::Separator();
        if (show_buttons("Tiles", tiles, &selected_tile)) {
            selected_tower = -1;
            selected_enemy = -1;
        }
        ImGui::Separator();
        if (show_buttons("Enemies", enemies, &selected_enemy)) {
            selected_tile = -1;
            selected_tower = -1;
        }
    } else {
        selected_tile = -1;
        selected_tower = -1;
        selected_enemy = -1;
    }
    ImGui::End();
}

}
