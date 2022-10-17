#include "imgui_extra.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "game/asset_browser.hpp"
#include "renderer/assets/asset_loader.hpp"


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



void InspectFile(const fs::path& path, fs::path* p_selected) {
    bool selected = p_selected ? *p_selected == path.string() : false;
    if (ImGui::Selectable(path.filename().string().c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_DontClosePopups, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight())))
        *p_selected = path.string();

    PathSource(path);
} 

void InspectDirectory(const fs::path& path, fs::path* p_selected, const std::function<bool(const fs::path&)>& filter, bool open_subdirectories) {
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
                InspectDirectory(dir_entry.path(), p_selected, filter, open_subdirectories);
        }
        for (auto& dir_entry : fs::directory_iterator(path)) {
            string dir_str = dir_entry.path().string();
            bool hidden = dir_entry.path().stem().string().starts_with('.');
            if (hidden)
                continue;

            if (dir_entry.is_regular_file() && filter(dir_entry.path()))
                InspectFile(dir_str, p_selected);
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

void PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, const string& dnd_key, bool open_subdirectories) {
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

void PathSelectBody(fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, bool* p_open, bool open_subdirectories) {
    spellbook::v2 size = p_open == nullptr ?
        spellbook::v2(ImGui::GetContentRegionAvail()) :
        spellbook::v2(ImGui::GetContentRegionAvail()) - spellbook::v2(0, ImGui::GetFrameHeightWithSpacing());
    ImGui::BeginChild("Directory", ImVec2(size));
    InspectDirectory(fs::path(base_folder), out, filter, open_subdirectories);
    ImGui::EndChild();

    if (p_open != nullptr)
        *p_open = ImGui::Button("Select", ImVec2(-FLT_MIN, 0));
}
