#include "stat.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/imgui_extra.hpp"

namespace spellbook {

    
f32 Stat::value() {
    f32 base = 0.0f;
    f32 mult = 1.0f;
    f32 add  = 0.0f;

    for (auto it = effects.begin(); it != effects.end();) {
        auto& [_, effect] = *it;
            
        if (effect.until < Input::time)
            it = effects.erase(it);
        else
            it++;
    }
    
    for (auto& [_, effect] : effects) {
        switch (effect.type) {
            case (StatEffect::Type_Base): {
                base += effect.value * effect.stacks;
            } break;
            case (StatEffect::Type_Multiply): {
                mult *= math::pow(1.0f + effect.value, (f32) effect.stacks);
            } break;
            case (StatEffect::Type_Add): {
                add += effect.value * effect.stacks;
            } break;
            case (StatEffect::Type_Override): {
                return effect.value;
            } break;
        }
    }
    return base * mult + add;
}

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
        ImGui::EnumCombo("##Effect", &effect.type);
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
