#include "map_editor.hpp"

#include <tracy/Tracy.hpp>

#include "lib_ext/fmt_geometry.hpp"

#include "scene.hpp"
#include "game.hpp"
#include "components.hpp"

#include "renderer/renderable.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

// clang-format off
vector<Brush> MapEditor::brushes = {
    {"Air", palette::black, nullptr, false},
    {"Floor", palette::green, nullptr, false},
    {"Path", palette::saddle_brown, nullptr, true},
    {"Spawner", palette::cadet_blue, nullptr, true},
    {"Consumer", palette::dark_gray, nullptr, true}
};
vector<Tower> MapEditor::towers = {};
// clang-format on

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

    MaterialCPU default_mat = {
        .name = "default",
        .file_name = "default",
        .base_color_tint = palette::black,
    };
    game.renderer.upload_material(default_mat, false);
}

void MapEditor::update() {
    ZoneScoped;
    Viewport& viewport = p_scene->render_scene.viewport;

    if (!viewport.hovered)
        return;

    v3  intersect = math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);
    v3i cell      = (v3i) math::floor(intersect);
    if (selected_brush != -1) {
        auto line_mesh = generate_formatted_line(p_scene->render_scene.viewport.camera,
        {
            {(v3) cell + v3(0.5, 0.0, 0.05), palette::white, 0.03f},
            {(v3) cell + v3(1.0, 0.0, 0.05), palette::white, 0.03f},
            {(v3) cell + v3(1.0, 1.0, 0.05), palette::white, 0.03f},
            {(v3) cell + v3(0.0, 1.0, 0.05), palette::white, 0.03f},
            {(v3) cell + v3(0.0, 0.0, 0.05), palette::white, 0.03f},
            {(v3) cell + v3(0.5, 0.0, 0.05), palette::white, 0.03f}});
        MeshGPU&     mesh     = game.renderer.upload_mesh(line_mesh);
        MaterialGPU& material = *game.renderer.get_material("default");
        // TODO: frame renderables
    } else if (selected_tower != -1) {
        vector<FormattedVertex> vertices;
        for (int i = 0; i <= 24; i++) {
            f32 angle  = i * math::TAU / 24.0f;
            v3  center = (v3) cell + v3(0.5f, 0.5f, 0.0f);
            vertices.emplace_back(center + 0.5f * v3(math::cos(angle), math::sin(angle), 0.05f), palette::white, 0.03f);
        }

        auto         line_mesh = generate_formatted_line(p_scene->render_scene.viewport.camera, std::move(vertices));
        MeshGPU&     mesh      = game.renderer.upload_mesh(line_mesh);
        MaterialGPU& material  = *game.renderer.get_material("default");
        // TODO: frame renderbales
    }

    // we should check if this mouse input is ours in the callback)
    if (Input::mouse_press_at[GLFW_MOUSE_BUTTON_LEFT] > 0.0f && selected_brush != -1) {
        auto& brush = brushes[selected_brush];

        bool found = false;
        auto view  = p_scene->registry.view<GridSlot, Model>();
        for (auto [entity, obj, model] : view.each()) {
            if (obj.position == cell) {
                found = true;
                // TODO: remove renderables
                p_scene->registry.destroy(entity);
                break;
            }
        }

        if (brush.name == "Air")
            return;

        static int i      = 0;
        auto       entity = p_scene->registry.create();
        p_scene->registry.emplace<Name>(entity, fmt_("{}_{}", brush.name, i++));
        p_scene->registry.emplace<Transform>(entity);
        // TODO: add brush
        // p_scene->registry.emplace<Model>(entity, renderables, v3(0.0f));
        p_scene->registry.emplace<GridSlot>(entity, cell, brush.travelable);
        // if (brush.name == "Spawner")
        //     p_scene->registry.emplace<Spawner>(entity, Input::time, 2.0f, for_spawner);
        if (brush.name == "Consumer")
            p_scene->registry.emplace<Consumer>(entity, 0.5f, 0);
    } else if (Input::mouse_press_at[GLFW_MOUSE_BUTTON_LEFT] > 0.0f && selected_tower != -1) {
        auto& tower = towers[selected_tower];

        auto view = p_scene->registry.view<Transform>();
        for (auto [entity, transform] : view.each()) {
            if (!p_scene->registry.any_of<Pyro, Roller>(entity))
                continue;
            if (math::length(transform.translation.xy - (v2) cell.xy) < 0.1f) {
                p_scene->registry.destroy(entity);
                break;
            }
        }

        static int i      = 0;
        auto       entity = p_scene->registry.create();
        p_scene->registry.emplace<Name>(entity, fmt_("{}_{}", tower.name, i++));
        p_scene->registry.emplace<Transform>(entity, v3(cell));
        // TODO: add tower
        // p_scene->registry.emplace<Model>(entity, renderables, v3(0.5f, 0.5f, 0.0f));
        if (selected_tower == 1) {
            p_scene->registry.emplace<Roller>(entity, Input::time, 4.0f, 0.3f, 1.5f, 0.3f, 2.0f);
        } else {
            p_scene->registry.emplace<Pyro>(entity, 2.0f, Input::time, 1.0f, 0.2f);
        }
    } else if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->render_scene.query = v2i(Input::mouse_pos) - p_scene->render_scene.viewport.start;
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

        ImGui::Text("Towers");
        int    towers_count      = towers.size();
        float  window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        ImVec2 tower_button_sz   = ImVec2(100, 30);
        for (int i = 0; i < towers_count; i++) {
            Color normal_color  = towers[i].button_color;
            Color hovered_color = mix(normal_color, palette::white, 0.2);
            Color pressed_color = mix(normal_color, palette::white, 0.1);

            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) normal_color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) hovered_color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) pressed_color);
            if (ImGui::Button(towers[i].name.c_str(), tower_button_sz) && selected_tower != i) {
                selected_tower = i;
                selected_brush = -1;
            }
            ImGui::PopStyleColor(3);

            if (selected_tower == i) {
                v2 min = v2(ImGui::GetItemRectMin()) - v2(1);
                v2 max = v2(ImGui::GetItemRectMax()) + v2(1);

                ImGui::GetForegroundDrawList()->AddRect((ImVec2) min, (ImVec2) max, (u32) palette::white, 0.0f, 0, 2.0f);
            }
            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + tower_button_sz.x;
            // Expected position if next button was on same line
            if (i + 1 < towers_count && next_button_x2 < window_visible_x2)
                ImGui::SameLine();
        }

        ImGui::Separator();
        ImGui::Text("Brushes");
        int    buttons_count = brushes.size();
        ImVec2 button_sz     = ImVec2(100, 100);
        for (int i = 0; i < buttons_count; i++) {
            Color normal_color  = brushes[i].button_color;
            Color hovered_color = mix(normal_color, palette::white, 0.2);
            Color pressed_color = mix(normal_color, palette::white, 0.1);

            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) normal_color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) hovered_color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) pressed_color);
            if (ImGui::Button(brushes[i].name.c_str(), button_sz) && selected_brush != i) {
                selected_brush = i;
                selected_tower = -1;
            }
            ImGui::PopStyleColor(3);

            if (selected_brush == i) {
                v2 min = v2(ImGui::GetItemRectMin()) - v2(1);
                v2 max = v2(ImGui::GetItemRectMax()) + v2(1);

                ImGui::GetForegroundDrawList()->AddRect((ImVec2) min, (ImVec2) max, (u32) palette::white, 0.0f, 0, 2.0f);
            }

            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line
            if (i + 1 < buttons_count && next_button_x2 < window_visible_x2)
                ImGui::SameLine();
        }
    } else {
        selected_brush = -1;
        selected_tower = -1;
    }
    ImGui::End();
}

}
