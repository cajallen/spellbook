#pragma once

#include <filesystem>
#include <functional>
#include <type_traits>

#include <imgui.h>
#include <magic_enum.hpp>

#include "general/file.hpp"
#include "general/matrix.hpp"
#include "general/string.hpp"
#include "general/vector.hpp"
#include "game/game_file.hpp"
#include "icons/font_awesome4.h"


namespace fs = std::filesystem;


namespace ImGui {

bool DragMat3(const string& name, spellbook::m33* matrix, f32 speed, const string& format);
bool DragMat4(const string& name, spellbook::m44* matrix, f32 speed, const string& format);
bool DragMat4(const string& name, spellbook::m44GPU* matrix, f32 speed, const string& format);

// Shows a selectable of path
bool InspectFile(const fs::path& path, fs::path* p_selected, const std::function<void(const fs::path&)>& context_callback = {});

// Shows a selectable treenode of path, recurses directory contents
bool InspectDirectory(const fs::path& path, fs::path* p_selected, const std::function<bool(const fs::path&)>& filter, int open_subdirectories = 1, const std::function<void(const fs::path&)>& context_callback = {});

// Enables the last item as a drag and drop source
void PathSource(const fs::path& in_path, string dnd_key = "");

// Enables the last item as a drag and drop target
void PathTarget(fs::path* out, const string& dnd_key);

// A widget to select paths
bool PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, spellbook::FileType file_type, int open_subdirectories = 1, const std::function<void(const fs::path&)>& context_callback = {});
bool PathSelect(const string& hint, string* out, const string& base_folder, spellbook::FileType file_type, int open_subdirectories = 1, const std::function<void(const fs::path&)>& context_callback = {});

bool PathSelectBody(fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, bool* p_open = nullptr, int open_subdirectories = 1, const std::function<void(const fs::path&)>& context_callback = {});

template <typename J>
concept combo_enum_concept = std::is_enum_v<J>;
template <combo_enum_concept T>
bool EnumCombo(const string& label, T* value, ImGuiComboFlags flags = 0) {
    bool ret = false;
    if (BeginCombo(label.c_str(), magic_enum::enum_name(*value).data(), flags)) {
        for (int i = 0; i < magic_enum::enum_count<T>(); i++) {
            const bool is_selected = *value == T(i);
            if (Selectable(magic_enum::enum_name((T) i).data(), is_selected)) {
                *value = T(i);
                ret = true;
            }
            if (is_selected)
                ::ImGui::SetItemDefaultFocus();
        }
        ::ImGui::EndCombo();
    }
    return ret;
}

void StyleColorsSpellbook(ImGuiStyle* dst = nullptr);

bool DragEuler2(const char* label, spellbook::euler* e, bool input_is_radians = true, float v_speed = 0.5f, ImGuiSliderFlags flags = ImGuiSliderFlags_NoRoundToFormat);
bool DragEuler3(const char* label, spellbook::euler* e, bool input_is_radians = true, float v_speed = 0.5f, ImGuiSliderFlags flags = ImGuiSliderFlags_NoRoundToFormat);

template<typename T, typename InspectFunc, typename InsertFunc>
bool OrderedVector(spellbook::vector<T>& values, InspectFunc callback, InsertFunc insert_callback, bool show_indices) {
    bool changed = false;

    bool insert_button = true;
    if constexpr(std::is_constructible<bool, InsertFunc>::value) {
        if (!insert_callback)
            insert_button = false;
    }
    if (insert_button) {
        bool pressed = false;
        if (ImGui::Button("  " ICON_FA_PLUS "  Add  ")) {
            pressed = true;
            changed = true;
        }
        insert_callback(values, pressed);
    }
    
    if (ImGui::BeginTable("vector_table", show_indices ? 5 : 4)) {
        if (show_indices)
            ImGui::TableSetupColumn("indices", ImGuiTableColumnFlags_WidthFixed, 16.0f);
        ImGui::TableSetupColumn("inspect");
        ImGui::TableSetupColumn("up", ImGuiTableColumnFlags_WidthFixed, 28.0f);
        ImGui::TableSetupColumn("down", ImGuiTableColumnFlags_WidthFixed, 28.0f);
        ImGui::TableSetupColumn("remove", ImGuiTableColumnFlags_WidthFixed, 28.0f);
        for (int i = 0; i < values.size(); i++) {
            ImGui::PushID(i);
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            if (show_indices) {
                ImGui::Text("%d", i);
                ImGui::TableNextColumn();
            }

            changed |= callback(values[i]);
            ImGui::TableNextColumn();

            if (i > 0) {
                if (ImGui::Button(ICON_FA_ARROW_UP, {24.f, 0.f})) {
                    std::swap(values[i-1], values[i]);
                    changed = true;
                }
            }
            ImGui::TableNextColumn();

            if (i + 1 < values.size()) {
                if (ImGui::Button(ICON_FA_ARROW_DOWN, {24.f, 0.f})) {
                    std::swap(values[i], values[i+1]);
                    changed = true;
                }
            }
            ImGui::TableNextColumn();

            if (ImGui::Button(ICON_FA_TIMES, {24.f, 0.f})) {
                values.remove_index(i, false);
                changed = true;
                i--;
            }
            
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    return changed;
}

template<typename T, typename InspectFunc, typename InsertFunc>
bool UnorderedVector(spellbook::vector<T>& values, InspectFunc callback, InsertFunc insert_callback, bool show_indices) {
    bool changed = false;

    bool pressed = false;
    if (ImGui::Button("  " ICON_FA_PLUS "  Add  ")) {
        pressed = true;
        changed = true;
    }
    insert_callback(values, pressed);

    float table_width = ImGui::GetContentRegionAvail().x;
    if (ImGui::BeginTable("vector_table", show_indices ? 3 : 2, 0, ImVec2{table_width, 0})) {
        if (show_indices)
            ImGui::TableSetupColumn("indices", ImGuiTableColumnFlags_WidthFixed, 16.0f);
        ImGui::TableSetupColumn("inspect", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("remove", ImGuiTableColumnFlags_WidthFixed, 28.0f);
        for (int i = 0; i < values.size(); i++) {
            ImGui::PushID(i);
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            if (show_indices) {
                ImGui::Text("%d", i);
                ImGui::TableNextColumn();
            }

            changed |= callback(values[i]);
            ImGui::TableNextColumn();

            if (ImGui::Button(ICON_FA_TIMES, {24.f, 0.f})) {
                values.remove_index(i, false);
                changed = true;
                i--;
            }
            
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    return changed;
}


}

