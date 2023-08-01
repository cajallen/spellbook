#include "stat.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/imgui_extra.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "renderer/assets/particles.hpp"

namespace spellbook {

float stat_instance_value(Stat* stat, float instance_base) {
    StatInstance instance {stat, instance_base};
    return instance.value();
}

void Stat::add_effect(uint64 id, const StatEffect& init_effect, EmitterCPU* emitter) {
    bool adding = false;
    if (!effects.contains(id)) {
        effects[id] = init_effect;
        effects[id].stacks = 0;
        adding = true;
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

    if (emitter && adding) {
        EmitterComponent& emitters = scene->registry.get<EmitterComponent>(entity);
        emitters.add_emitter(id, *emitter);
    }
}

umap<uint64, StatEffect>::iterator Stat::remove_effect(umap<uint64, StatEffect>::iterator it) {
    EmitterComponent* emitters = scene->registry.try_get<EmitterComponent>(entity);
    if (emitters)
        emitters->remove_emitter(it->first);

    return effects.erase(it);
}

void Stat::remove_effect(uint64 id) {
    if (effects.contains(id)) {
        EmitterComponent* emitters = scene->registry.try_get<EmitterComponent>(entity);
        if (emitters)
            emitters->remove_emitter(id);

        effects.erase(id);
    }
}


float Stat::value() {
    float base = 0.0f;
    float mult = 1.0f;
    float add  = 0.0f;

    for (auto it = effects.begin(); it != effects.end();) {
        auto& [_, effect] = *it;
            
        if (effect.until < scene->time)
            it = remove_effect(it);
        else
            it++;
    }
    
    for (auto& [_, effect] : effects) {
        switch (effect.type) {
            case (StatEffect::Type_Base): {
                base += effect.value * effect.stacks;
            } break;
            case (StatEffect::Type_Multiply): {
                mult *= math::pow(1.0f + effect.value, (float) effect.stacks);
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

float StatInstance::value() const {
    if (stat == nullptr)
        return 0.0f;
    
    float base = instance_base;
    float mult = 1.0f;
    float add  = 0.0f;

    for (auto it = stat->effects.begin(); it != stat->effects.end();) {
        auto& [_, effect] = *it;
            
        if (effect.until < stat->scene->time)
            it = stat->remove_effect(it);
        else
            it++;
    }
    
    for (auto& [_, effect] : stat->effects) {
        switch (effect.type) {
            case (StatEffect::Type_Base): {
                base += effect.value * effect.stacks;
            } break;
            case (StatEffect::Type_Multiply): {
                mult *= math::pow(1.0f + effect.value, (float) effect.stacks);
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
