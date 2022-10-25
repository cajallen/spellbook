#pragma once

#include <filesystem>
#include <functional>
#include <type_traits>

#include <imgui.h>
#include <magic_enum.hpp>

#include "matrix.hpp"
#include "string.hpp"


namespace fs = std::filesystem;

bool DragMat3(const string& name, spellbook::m33* matrix, f32 speed, const string& format);
bool DragMat4(const string& name, spellbook::m44* matrix, f32 speed, const string& format);

// Shows a selectable of path
void InspectFile(const fs::path& path, fs::path* p_selected, const std::function<void(const fs::path&)>& context_callback = {});

// Shows a selectable treenode of path, recurses directory contents
void InspectDirectory(const fs::path& path, fs::path* p_selected, const std::function<bool(const fs::path&)>& filter, bool open_subdirectories = true, const std::function<void(const fs::path&)>& context_callback = {});

// Enables the last item as a drag and drop source
void PathSource(const fs::path& in_path, string dnd_key = "");

// Enables the last item as a drag and drop target
void PathTarget(fs::path* out, const string& dnd_key);

// A widget to select paths
void PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, const string& dnd_key, bool open_subdirectories = true, const std::function<void(const fs::path&)>& context_callback = {});

void PathSelectBody(fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, bool* p_open = nullptr, bool open_subdirectories = true, const std::function<void(const fs::path&)>& context_callback = {});

template <typename J>
concept enum_concept = std::is_enum_v<J>;
template <enum_concept T>
bool EnumCombo(const string& label, T* value, ImGuiComboFlags flags = 0) {
    bool ret = false;
    if (ImGui::BeginCombo(label.c_str(), magic_enum::enum_name(*value).data(), flags)) {
        for (int i = 0; i < magic_enum::enum_count<T>(); i++) {
            const bool is_selected = *value == T(i);
            if (ImGui::Selectable(magic_enum::enum_name((T) i).data(), is_selected)) {
                *value = T(i);
                ret = true;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return ret;
}

namespace spellbook {

void StyleColorsSpellbook(ImGuiStyle* dst = NULL);

}
