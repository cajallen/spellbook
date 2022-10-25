#pragma once

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "color.hpp"
#include "vector.hpp"
#include "scene.hpp"

#include "game/tower.hpp"
#include "game/tile.hpp"
#include "game/enemy.hpp"

namespace spellbook {

template <typename T>
struct Button {
    string text;
    Color color;
    T item;
};

struct MapEditor {
    vector<Button<TowerPrefab>> towers;
    vector<Button<TilePrefab>>  tiles;
    vector<Button<EnemyPrefab>> enemies;
    
    u32 selected_tower = -1;
    u32 selected_tile = -1;
    u32 selected_enemy = -1;
    Scene* p_scene        = nullptr;

    string browser_string = "C:/spellbook";
    
    void   setup();
    void   setup(Scene* init_scene);
    void   update();
    void   window(bool* p_open);
};

template <typename T>
bool show_buttons(const string& name, vector<Button<T>>& buttons, u32* selected) {
    static umap<string, Button<T>> add_map;
    
    bool ret = false;

    ImGui::PushID(name.c_str());
    
    ImGui::Text("%s", name.c_str());

    if (!add_map.contains(name)) add_map[name] = {};

    if (ImGui::Button("Add")) {
        ImGui::OpenPopup("Add Button");
    }

    if (ImGui::BeginPopupModal("Add Button")) {
        ImGui::InputText("Text", &add_map[name].text);
        ImGui::ColorEdit3("Color", add_map[name].color.data);
        inspect(&add_map[name].item);

        if (ImGui::Button("Add")) {
            buttons.emplace_back(std::move(add_map[name]));
            add_map[name] = {};
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    float  window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    ImVec2 button_size   = ImVec2(100, 30);
    for (int i = 0; i < buttons.size(); i++) {
        Color normal_color  = buttons[i].color;
        Color hovered_color = mix(normal_color, palette::white, 0.2f);
        Color pressed_color = mix(normal_color, palette::white, 0.1f);

        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) normal_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) hovered_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) pressed_color);
        if (ImGui::Button(buttons[i].text.c_str(), button_size) && *selected != i) {
            *selected = i;
            ret = true;
        }
        ImGui::PopStyleColor(3);

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

    ImGui::PopID();
    return ret;
}

}
