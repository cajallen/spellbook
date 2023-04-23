#include "stat.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/imgui_extra.hpp"
#include "game/scene.hpp"

namespace spellbook {

float stat_instance_value(Stat* stat, float instance_base) {
    StatInstance instance {stat, instance_base};
    return instance.value();
}

void Stat::add_effect(u64 id, const StatEffect& init_effect) {
    if (!effects.contains(id)) {
        effects[id] = init_effect;
        effects[id].stacks = 0;
    }

    StatEffect& effect_value = effects[id];
    
    // Overwrite existing values, may way to assert equal
    {
        effect_value.type = init_effect.type;
        effect_value.value = init_effect.value;
    }

    // Refresh and add stacks
    effect_value.stacks = math::min(effect_value.stacks + init_effect.stacks, effect_value.max_stacks);
    effect_value.until = scene->time + init_effect.until;
}

void Stat::remove_effect(u64 id) {
    if (effects.contains(id)) {
        effects.erase(id);
    } 
}


f32 Stat::value() {
    f32 base = 0.0f;
    f32 mult = 1.0f;
    f32 add  = 0.0f;

    for (auto it = effects.begin(); it != effects.end();) {
        auto& [_, effect] = *it;
            
        if (effect.until < scene->time)
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

f32 StatInstance::value() {
    if (stat == nullptr)
        return 0.0f;
    
    f32 base = instance_base;
    f32 mult = 1.0f;
    f32 add  = 0.0f;

    for (auto it = stat->effects.begin(); it != stat->effects.end();) {
        auto& [_, effect] = *it;
            
        if (effect.until < stat->scene->time)
            it = stat->effects.erase(it);
        else
            it++;
    }
    
    for (auto& [_, effect] : stat->effects) {
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
    
    for (auto& [id, effect] : stat->effects) {
        ImGui::PushID(&effect);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%llu", id);
        ImGui::TableSetColumnIndex(1);
        ImGui::EnumCombo("##Effect", &effect.type);
        ImGui::TableSetColumnIndex(2);
        ImGui::DragFloat("##Value", &effect.value, 0.01f);
        ImGui::PopID();
    }
    ImGui::EndTable();
    ImGui::PopID();
}

}
