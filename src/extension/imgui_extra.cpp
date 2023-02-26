#include "imgui_extra.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <imgui_internal.h>

#include "extension/icons/font_awesome4.h"
#include "editor/asset_browser.hpp"

namespace ImGui {

bool DragMat3(const string& name, spellbook::m33* matrix, f32 speed, const string& format) {
    bool changed = false;

    BeginGroup();
    PushID(name.c_str());
    Text("%s", name.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat3("##row0", &matrix->cr(0, 0), speed, 0.0f, 0.0f, format.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat3("##row1", &matrix->cr(0, 1), speed, 0.0f, 0.0f, format.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat3("##row2", &matrix->cr(0, 2), speed, 0.0f, 0.0f, format.c_str());
    PopID();
    EndGroup();

    return changed;
}

bool DragMat4(const string& name, spellbook::m44* matrix, f32 speed, const string& format) {
    bool changed = false;

    BeginGroup();
    PushID(name.c_str());
    Text("%s", name.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat4("##row0", &matrix->cr(0, 0), speed, 0.0f, 0.0f, format.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat4("##row1", &matrix->cr(0, 1), speed, 0.0f, 0.0f, format.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat4("##row2", &matrix->cr(0, 2), speed, 0.0f, 0.0f, format.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat4("##row3", &matrix->cr(0, 3), speed, 0.0f, 0.0f, format.c_str());
    PopID();
    EndGroup();

    return changed;
}

bool DragMat4(const string& name, spellbook::m44GPU* matrix_gpu, f32 speed, const string& format) {
    bool changed = false;

    spellbook::m44 matrix = (spellbook::m44) *matrix_gpu;
    
    BeginGroup();
    PushID(name.c_str());
    Text("%s", name.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat4("##row0", &matrix.cr(0, 0), speed, 0.0f, 0.0f, format.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat4("##row1", &matrix.cr(0, 1), speed, 0.0f, 0.0f, format.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat4("##row2", &matrix.cr(0, 2), speed, 0.0f, 0.0f, format.c_str());
    SetNextItemWidth(-FLT_MIN);
    changed |= DragFloat4("##row3", &matrix.cr(0, 3), speed, 0.0f, 0.0f, format.c_str());
    PopID();
    EndGroup();

    if (changed) {
        *matrix_gpu = (spellbook::m44GPU) matrix;
    }
    
    return changed;
}


bool InspectFile(const fs::path& path, fs::path* p_selected, const std::function<void(const fs::path&)>& context_callback) {
    bool changed = false;
    bool selected = p_selected ? *p_selected == path.string() : false;
    if (Selectable(path.filename().string().c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_DontClosePopups, ImVec2(GetContentRegionAvail().x, GetFrameHeight()))) {
        *p_selected = path.string();
        changed = true;
    }

    if (context_callback && BeginPopupContextItem()) {
        context_callback(path);
        EndPopup();
    }
    
    PathSource(path);
    return changed;
} 

bool InspectDirectory(const fs::path& path, fs::path* p_selected, const std::function<bool(const fs::path&)>& filter, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    bool changed = false;
    string folder_name = path.string();
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (p_selected && folder_name == *p_selected)
        node_flags |= ImGuiTreeNodeFlags_Selected;

    SetNextItemOpen(open_subdirectories > 0, ImGuiCond_Once);

    string dir_string = path.has_stem() ? path.stem().string() : folder_name;
    
    bool node_open = TreeNodeEx(dir_string.c_str(), node_flags);
    if (IsItemClicked() && !IsItemToggledOpen() && p_selected) {
        *p_selected = folder_name;
        changed = true;
    }
    
    PathSource(folder_name);

    if (node_open) {
        for (auto& dir_entry : fs::directory_iterator(path)) {
            string dir_string = dir_entry.path().string();
            bool   hidden     = dir_entry.path().stem().string().starts_with('.');
            if (hidden && dir_string != ".")
                continue;

            if (dir_entry.is_directory())
                changed |= InspectDirectory(dir_entry.path(), p_selected, filter, open_subdirectories - 1, context_callback);
        }
        for (auto& dir_entry : fs::directory_iterator(path)) {
            string dir_str = dir_entry.path().string();
            bool hidden = dir_entry.path().stem().string().starts_with('.');
            if (hidden)
                continue;

            if (dir_entry.is_regular_file() && filter(dir_entry.path()))
                changed |= InspectFile(dir_str, p_selected, context_callback);
        }
        TreePop();
    }
    return changed;
}


void PathSource(const fs::path& in_path, string dnd_key_string) {
    if (dnd_key_string.empty()) {
        magic_enum::enum_for_each<spellbook::FileType>([&in_path, &dnd_key_string] (auto val) {
            if (path_filter(val)(in_path))
                dnd_key_string = dnd_key(val);
        });
    }
    
    if (BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        SetDragDropPayload(dnd_key_string.c_str(), in_path.string().c_str(), in_path.string().size());
        Text("%s", in_path.string().c_str());
        EndDragDropSource();
    }
}

void PathTarget(fs::path* out, const string& dnd_key) {
    if (BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = AcceptDragDropPayload(dnd_key.c_str())) {
            string out_string;
            out_string.resize(payload->DataSize);
            strncpy(out_string.data(), (char*) payload->Data, payload->DataSize);
            *out = fs::path(out_string);
        }
        EndDragDropTarget();
    }
}

bool PathSelect(const string& hint, string* out, const string& base_folder, spellbook::FileType file_type, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    fs::path out_input = fs::path(*out);
    bool changed = PathSelect(hint, &out_input, base_folder, file_type, open_subdirectories, context_callback);
    *out = out_input.string();
    return changed;
}

bool PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, spellbook::FileType file_type, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    if (!fs::exists(base_folder))
        fs::create_directory(base_folder);
    
    bool changed = false;
    PushID(hint.c_str());
    BeginGroup();
    {
        if (Button(ICON_FA_FOLDER))
            OpenPopup("File Select");
        SameLine();
        string out_string = out->string();
        if (InputText(hint.c_str(), &out_string)) {
            *out = fs::path(out_string);
            changed = true;
        }
    }
    EndGroup();
    PathTarget(out, dnd_key(file_type));

    bool open = true;
    
    SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    SetNextWindowPos(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    if (BeginPopupModal("File Select", &open)) {
        bool close_popup = false;
        changed |= PathSelectBody(out, base_folder, path_filter(file_type), &close_popup, open_subdirectories, context_callback);
        if (close_popup)
            CloseCurrentPopup();
        EndPopup();
    }
    PopID();
    
    return changed;
}

bool PathSelectBody(fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, bool* close_popup, int open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    spellbook::v2 size = close_popup == nullptr ?
        spellbook::v2(GetContentRegionAvail()) :
        spellbook::v2(GetContentRegionAvail()) - spellbook::v2(0, GetFrameHeightWithSpacing());
    BeginChild("Directory", ImVec2(size));
    SetNextItemOpen(true, ImGuiCond_Appearing);
    bool changed = InspectDirectory(fs::path(base_folder), out, filter, open_subdirectories, context_callback);
    EndChild();

    if (close_popup != nullptr)
        *close_popup = Button("Select", ImVec2(-FLT_MIN, 0));

    return changed;
}

void StyleColorsSpellbook(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = (ImVec4) spellbook::Color(spellbook::palette::white, 1.0f);
    colors[ImGuiCol_TextDisabled]           = (ImVec4) spellbook::Color(spellbook::palette::gray, 1.0f); // ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // grey
    colors[ImGuiCol_WindowBg]               = (ImVec4) spellbook::Color(spellbook::palette::near_black, 0.96f); // ImVec4(0.06f, 0.06f, 0.06f, 0.94f); // dark
    colors[ImGuiCol_ChildBg]                = (ImVec4) spellbook::Color(spellbook::palette::black, 0.0f); // ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // empty
    colors[ImGuiCol_PopupBg]                = (ImVec4) spellbook::Color(spellbook::palette::near_black, 0.95f); // ImVec4(0.08f, 0.08f, 0.08f, 0.94f); // dark
    colors[ImGuiCol_Border]                 = (ImVec4) spellbook::Color(spellbook::palette::spellbook_gray, 0.5f); // ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = (ImVec4) spellbook::Color(spellbook::palette::black, 0.0f); // ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = (ImVec4) spellbook::Color(spellbook::palette::spellbook_7, 0.6f); // ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.4f); // ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.7f); // ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = (ImVec4) spellbook::Color(spellbook::palette::near_black, 1.0f); // ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = (ImVec4) spellbook::Color(spellbook::palette::spellbook_7, 1.0f); // ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4) spellbook::Color(spellbook::palette::black, 0.5f); // ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = (ImVec4) spellbook::Color(spellbook::palette::gray_1, 1.0f); // ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4) spellbook::Color(spellbook::palette::black, 0.5f); // ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4) spellbook::Color(spellbook::palette::gray_3, 1.0f); // ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4) spellbook::Color(spellbook::palette::gray_4, 1.0f); // ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4) spellbook::Color(spellbook::palette::gray_5, 1.0f); // ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = (ImVec4) spellbook::Color(spellbook::palette::spellbook_5, 1.0f); // ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.4f); // ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = (ImVec4) spellbook::Color(spellbook::palette::spellbook_6, 1.0f); // ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.3f); // ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.8f); // ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4) spellbook::Color(spellbook::palette::spellbook_7, 0.8f); // ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = (ImVec4) spellbook::Color(spellbook::palette::spellbook_7, 1.0f); // ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.2f); // ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.7f); // ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.9f); // ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_DockingPreview]         = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.7f);
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4) spellbook::Color(spellbook::palette::gray_2, 1.0f); // ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = (ImVec4) spellbook::Color(spellbook::palette::gray_6, 1.0f); // ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4) spellbook::Color(spellbook::palette::tomato, 1.0f); // ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = (ImVec4) spellbook::Color(spellbook::palette::dark_orange, 1.0f); // ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4) spellbook::Color(spellbook::palette::orange, 1.0f); // ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4) spellbook::Color(spellbook::palette::gray_2, 1.0f); // ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4) spellbook::Color(spellbook::palette::gray_3, 1.0f); // ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = (ImVec4) spellbook::Color(spellbook::palette::gray_2, 1.0f); // ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = (ImVec4) spellbook::Color(spellbook::palette::black, 0.0f); // ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4) spellbook::Color(spellbook::palette::white, 0.05f); // ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 0.3f); // ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = (ImVec4) spellbook::Color(spellbook::palette::yellow_green, 0.8f); // ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = (ImVec4) spellbook::Color(spellbook::palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4) spellbook::Color(spellbook::palette::white, 0.7f); // ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4) spellbook::Color(spellbook::palette::gray_8, 0.2f); // ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4) spellbook::Color(spellbook::palette::gray_8, 0.4f); // ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
}

bool DragEuler2(const char* label, spellbook::euler* e, bool input_is_radians, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
    if (input_is_radians) {
        spellbook::euler e_deg = input_is_radians ? spellbook::math::r2d(*e) : *e;
        bool changed = ImGui::DragFloat2(label, &e_deg.yaw, v_speed, v_min, v_max, format, flags);
        if (changed) {
            *e = spellbook::math::d2r(e_deg);
            return true;
        }
        return false;
    }
    return ImGui::DragFloat2(label, &e->yaw, v_speed, v_min, v_max, format, flags);
}

bool DragEuler3(const char* label, spellbook::euler* e, bool input_is_radians, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
    if (input_is_radians) {
        spellbook::euler e_deg = input_is_radians ? spellbook::math::r2d(*e) : *e;
        bool changed = ImGui::DragFloat3(label, &e_deg.yaw, v_speed, v_min, v_max, format, flags);
        if (changed) {
            *e = spellbook::math::d2r(e_deg);
            return true;
        }
        return false;
    }
    return ImGui::DragFloat3(label, &e->yaw, v_speed, v_min, v_max, format, flags);
}

}



