#pragma once

#include <imgui.h>

#include "matrix.hpp"
#include "string.hpp"

inline bool DragMat3(const string& name, m33* matrix, f32 speed, const string& format) {
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

inline bool DragMat4(const string& name, m44* matrix, f32 speed, const string& format) {
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