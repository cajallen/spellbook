#include "stat.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "lib_ext/imgui_extra.hpp"

namespace spellbook {

void inspect(Stat* stat) {
    ImGui::Text("Value: %.2f", stat->value());

    ImGui::PushID(stat);
    ImGui::BeginTable("StatTable", 3);
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Type");
    ImGui::TableSetupColumn("Value");
    ImGui::TableHeadersRow();

    string name_from;
    string name_to;
    
    for (auto& [name, effect] : stat->effects) {
        ImGui::PushID(&effect);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        string name_copy = name;
        if (ImGui::InputText("##Name", &name_copy)) {
            name_from = name;
            name_to = name_copy;
        }
        ImGui::TableSetColumnIndex(1);
        EnumCombo("##Effect", &effect.type);
        ImGui::TableSetColumnIndex(2);
        ImGui::DragFloat("##Value", &effect.value, 0.01f);
        ImGui::PopID();
    }
    ImGui::EndTable();
    if (name_from != name_to) {
        std::swap(stat->effects[name_to], stat->effects[name_from]);
        stat->effects.erase(name_from);
    }
    ImGui::PopID();
}

}
