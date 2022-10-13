#include "imgui_extra.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

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
    if (ImGui::Selectable(path.string().c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_DontClosePopups, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight())))
        *p_selected = path.string();

    PathSource(path, "DND_FILE");
} 

void InspectDirectory(const fs::path& path, fs::path* p_selected) {
    string folder_name = path.string();
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (p_selected && folder_name == *p_selected)
        node_flags |= ImGuiTreeNodeFlags_Selected;
    bool node_open = ImGui::TreeNodeEx(folder_name.c_str(), node_flags);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && p_selected)
        *p_selected = folder_name;
    PathSource(folder_name, "DND_FOLDER");
    if (node_open) {
        for (auto& dir_entry : fs::directory_iterator(path)) {
            string dir_string = dir_entry.path().string();
            bool   hidden     = dir_entry.path().stem().string().starts_with('.');
            if (hidden && dir_string != ".")
                continue;

            if (dir_entry.is_directory())
                InspectDirectory(dir_entry.path(), p_selected);
        }
        for (auto& dir_entry : fs::directory_iterator(path)) {
            string dir_str = dir_entry.path().string();
            bool hidden = dir_entry.path().stem().string().starts_with('.');
            if (hidden)
                continue;

            if (dir_entry.is_regular_file())
                InspectFile(dir_str, p_selected);
        }
        ImGui::TreePop();
    }
}


void PathSource(const fs::path& in_path, const string& dnd_key) {
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

void PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, const string& dnd_key){
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
        spellbook::v2 size = spellbook::v2(ImGui::GetContentRegionAvail()) - spellbook::v2(0, ImGui::GetFrameHeightWithSpacing());
        ImGui::BeginChild("Directory", ImVec2(size));
        InspectDirectory(fs::path(base_folder), out);
        ImGui::EndChild();
        
        if (ImGui::Button("Select", ImVec2(-FLT_MIN, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopID();
}
