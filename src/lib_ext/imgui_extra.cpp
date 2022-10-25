#include "imgui_extra.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "imgui_internal.h"
#include "game/asset_browser.hpp"
#include "renderer/assets/model.hpp"


bool DragMat3(const string& name, spellbook::m33* matrix, f32 speed, const string& format) {
    bool changed = false;

    ImGui::BeginGroup();
    ImGui::PushID(name.c_str());
    ImGui::Text("%s", name.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::DragFloat3("##row0", &matrix->cr(0, 0), speed, 0.0f, 0.0f, format.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::DragFloat3("##row1", &matrix->cr(0, 1), speed, 0.0f, 0.0f, format.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::DragFloat3("##row2", &matrix->cr(0, 2), speed, 0.0f, 0.0f, format.c_str());
    ImGui::PopID();
    ImGui::EndGroup();

    return changed;
}

bool DragMat4(const string& name, spellbook::m44* matrix, f32 speed, const string& format) {
    bool changed = false;

    ImGui::BeginGroup();
    ImGui::PushID(name.c_str());
    ImGui::Text("%s", name.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::DragFloat4("##row0", &matrix->cr(0, 0), speed, 0.0f, 0.0f, format.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::DragFloat4("##row1", &matrix->cr(0, 1), speed, 0.0f, 0.0f, format.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::DragFloat4("##row2", &matrix->cr(0, 2), speed, 0.0f, 0.0f, format.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::DragFloat4("##row3", &matrix->cr(0, 3), speed, 0.0f, 0.0f, format.c_str());
    ImGui::PopID();
    ImGui::EndGroup();

    return changed;
}



void InspectFile(const fs::path& path, fs::path* p_selected, const std::function<void(const fs::path&)>& context_callback) {
    bool selected = p_selected ? *p_selected == path.string() : false;
    if (ImGui::Selectable(path.filename().string().c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_DontClosePopups, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight())))
        *p_selected = path.string();

    if (context_callback && ImGui::BeginPopupContextItem()) {
        context_callback(path);
        ImGui::EndPopup();
    }
    
    PathSource(path);
} 

void InspectDirectory(const fs::path& path, fs::path* p_selected, const std::function<bool(const fs::path&)>& filter, bool open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    string folder_name = path.string();
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (p_selected && folder_name == *p_selected)
        node_flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::SetNextItemOpen(open_subdirectories, ImGuiCond_Appearing);

    string dir_string = path.has_stem() ? path.stem().string() : folder_name;
    
    bool node_open = ImGui::TreeNodeEx(dir_string.c_str(), node_flags);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && p_selected)
        *p_selected = folder_name;
    
    PathSource(folder_name);

    if (node_open) {
        for (auto& dir_entry : fs::directory_iterator(path)) {
            string dir_string = dir_entry.path().string();
            bool   hidden     = dir_entry.path().stem().string().starts_with('.');
            if (hidden && dir_string != ".")
                continue;

            if (dir_entry.is_directory())
                InspectDirectory(dir_entry.path(), p_selected, filter, open_subdirectories, context_callback);
        }
        for (auto& dir_entry : fs::directory_iterator(path)) {
            string dir_str = dir_entry.path().string();
            bool hidden = dir_entry.path().stem().string().starts_with('.');
            if (hidden)
                continue;

            if (dir_entry.is_regular_file() && filter(dir_entry.path()))
                InspectFile(dir_str, p_selected, context_callback);
        }
        ImGui::TreePop();
    }
}


void PathSource(const fs::path& in_path, string dnd_key) {
    if (dnd_key.empty()) {
        if (spellbook::possible_folder(in_path))
            dnd_key = "DND_FOLDER";
        if (spellbook::possible_file(in_path))
            dnd_key = "DND_UNKNOWN_FILE";
        if (spellbook::possible_model(in_path))
            dnd_key = "DND_MODEL";
        if (spellbook::possible_model_asset(in_path))
            dnd_key = "DND_MODEL_ASSET";
        if (spellbook::possible_texture(in_path))
            dnd_key = "DND_TEXTURE";
        if (spellbook::possible_texture_asset(in_path))
            dnd_key = "DND_TEXTURE_ASSET";
        if (spellbook::possible_mesh(in_path))
            dnd_key = "DND_MESH";
        if (spellbook::possible_material(in_path))
            dnd_key = "DND_MATERIAL";
    }
    
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload(dnd_key.c_str(), in_path.string().c_str(), in_path.string().size());
        ImGui::Text("%s", in_path.string().c_str());
        ImGui::EndDragDropSource();
    }
}

void PathTarget(fs::path* out, const string& dnd_key) {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dnd_key.c_str())) {
            string out_string;
            out_string.resize(payload->DataSize);
            strncpy(out_string.data(), (char*) payload->Data, payload->DataSize);
            *out = fs::path(out_string);
        }
        ImGui::EndDragDropTarget();
    }
}

void PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, const string& dnd_key, bool open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    ImGui::PushID(hint.c_str());
    ImGui::BeginGroup();
    {
        string out_string = out->string();
        if (ImGui::InputText(hint.c_str(), &out_string))
            *out = fs::path(out_string);
        ImGui::SameLine();
        if (ImGui::SmallButton("Select"))
            ImGui::OpenPopup("File Select");
    }
    ImGui::EndGroup();
    PathTarget(out, dnd_key);

    bool open = true;
    
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("File Select", &open)) {
        bool close_popup = false;
        PathSelectBody(out, base_folder, filter, &close_popup, open_subdirectories);
        if (close_popup)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopID();
}

void PathSelectBody(fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, bool* p_open, bool open_subdirectories, const std::function<void(const fs::path&)>& context_callback) {
    spellbook::v2 size = p_open == nullptr ?
        spellbook::v2(ImGui::GetContentRegionAvail()) :
        spellbook::v2(ImGui::GetContentRegionAvail()) - spellbook::v2(0, ImGui::GetFrameHeightWithSpacing());
    ImGui::BeginChild("Directory", ImVec2(size));
    InspectDirectory(fs::path(base_folder), out, filter, open_subdirectories, context_callback);
    ImGui::EndChild();

    if (p_open != nullptr)
        *p_open = ImGui::Button("Select", ImVec2(-FLT_MIN, 0));
}

namespace spellbook {

void StyleColorsSpellbook(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = (ImVec4) Color(palette::white, 1.0f);
    colors[ImGuiCol_TextDisabled]           = (ImVec4) Color(palette::gray, 1.0f); // ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // grey
    colors[ImGuiCol_WindowBg]               = (ImVec4) Color(palette::near_black, 0.95f); // ImVec4(0.06f, 0.06f, 0.06f, 0.94f); // dark
    colors[ImGuiCol_ChildBg]                = (ImVec4) Color(palette::black, 0.0f); // ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // empty
    colors[ImGuiCol_PopupBg]                = (ImVec4) Color(palette::near_black, 0.9f); // ImVec4(0.08f, 0.08f, 0.08f, 0.94f); // dark
    colors[ImGuiCol_Border]                 = (ImVec4) Color(palette::spellbook_gray, 0.5f); // ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = (ImVec4) Color(palette::black, 0.0f); // ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = (ImVec4) Color(palette::spellbook_7, 0.5f); // ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4) Color(palette::spellbook_4, 0.4f); // ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = (ImVec4) Color(palette::spellbook_4, 0.7f); // ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = (ImVec4) Color(palette::near_black, 1.0f); // ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = (ImVec4) Color(palette::spellbook_7, 1.0f); // ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4) Color(palette::black, 0.5f); // ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = (ImVec4) Color(palette::gray_1, 1.0f); // ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4) Color(palette::black, 0.5f); // ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4) Color(palette::gray_3, 1.0f); // ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4) Color(palette::gray_4, 1.0f); // ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4) Color(palette::gray_5, 1.0f); // ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = (ImVec4) Color(palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = (ImVec4) Color(palette::spellbook_5, 1.0f); // ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4) Color(palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = (ImVec4) Color(palette::spellbook_4, 0.4f); // ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = (ImVec4) Color(palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = (ImVec4) Color(palette::spellbook_6, 1.0f); // ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = (ImVec4) Color(palette::spellbook_4, 0.3f); // ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = (ImVec4) Color(palette::spellbook_4, 0.8f); // ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = (ImVec4) Color(palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4) Color(palette::spellbook_7, 0.8f); // ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = (ImVec4) Color(palette::spellbook_7, 1.0f); // ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = (ImVec4) Color(palette::spellbook_4, 0.2f); // ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4) Color(palette::spellbook_4, 0.7f); // ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4) Color(palette::spellbook_4, 0.9f); // ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_DockingPreview]         = (ImVec4) Color(palette::spellbook_4, 0.7f);
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4) Color(palette::gray_2, 1.0f); // ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = (ImVec4) Color(palette::gray_6, 1.0f); // ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4) Color(palette::tomato, 1.0f); // ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = (ImVec4) Color(palette::dark_orange, 1.0f); // ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4) Color(palette::orange, 1.0f); // ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4) Color(palette::gray_2, 1.0f); // ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4) Color(palette::gray_3, 1.0f); // ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = (ImVec4) Color(palette::gray_2, 1.0f); // ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = (ImVec4) Color(palette::black, 0.0f); // ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4) Color(palette::white, 0.05f); // ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4) Color(palette::spellbook_4, 0.3f); // ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = (ImVec4) Color(palette::yellow_green, 0.8f); // ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = (ImVec4) Color(palette::spellbook_4, 1.0f); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4) Color(palette::white, 0.7f); // ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4) Color(palette::gray_8, 0.2f); // ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4) Color(palette::gray_8, 0.3f); // ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
}

}
