#pragma once

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "color.hpp"
#include "consumer.hpp"
#include "vector.hpp"
#include "scene.hpp"
#include "file.hpp"

#include "map.hpp"
#include "spawner.hpp"
#include "game/tower.hpp"
#include "game/tile.hpp"
#include "game/enemy.hpp"
#include "game/components.hpp"

#include "lib_ext/imgui_extra.hpp"

namespace spellbook {

template <typename T>
struct Button {
    string text;
    Color color;
    string item_path;
};

struct MapEditor {
    MapPrefab map_prefab;

    vector<Button<ConsumerPrefab>> consumer_buttons;
    vector<Button<TowerPrefab>> tower_buttons;
    vector<Button<TilePrefab>>  tile_buttons;
    vector<Button<SpawnerPrefab>> spawner_buttons;

    bool eraser_selected = false;
    u32 selected_consumer = -1;
    u32 selected_tower = -1;
    u32 selected_tile  = -1;
    u32 selected_spawner = -1;
    Scene* p_scene       = nullptr;
    
    void setup();
    void update();
    void window(bool* p_open);
    void shutdown();

    void unselect_buttons();

    void draw_preview(v3i cell);
    
    template <typename T>
    void instance_and_write_prefab(const T& t, v3i pos) {
        entt::entity old_tile = p_scene->get(pos, (T*) 0);
        if (old_tile != entt::null) {
            p_scene->registry.destroy(old_tile);
        }
        
        map_prefab.add_prefab(t, pos);
        instance_prefab(p_scene, t, pos);
    }
};

JSON_IMPL_TEMPLATE(template <typename T>, Button<T>, text, color, item_path);

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

    bool add_open;
    if (ImGui::BeginPopupModal("Add Button", &add_open)) {
        ImGui::InputText("Text", &add_map[name].text);
        ImGui::ColorEdit3("Color", add_map[name].color.data);
        ImGui::PathSelect("Path", &add_map[name].item_path, "resources", from_typeinfo(typeid(T)), true);
        
        if (ImGui::Button("Add")) {
            buttons.emplace_back(std::move(add_map[name]));
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

}
