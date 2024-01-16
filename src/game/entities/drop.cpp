#include "drop.hpp"

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "extension/icons/font_awesome4.h"
#include "game/scene.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const BeadPrefab& bead_prefab, v3 position) {
    static int i      = 0;
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", bead_prefab.file_path.stem(), i++));
    
    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_resource<ModelCPU>(bead_prefab.model_path));
    model_comp.model_gpu = std::move(instance_model(scene->render_scene, *model_comp.model_cpu));

    scene->registry.emplace<LogicTransform>(entity, position);
    scene->registry.emplace<ModelTransform>(entity, v3{}, quat{}, v3(bead_prefab.scale));
    scene->registry.emplace<TransformLink>(entity, v3(0.5));

    scene->registry.emplace<Pickup>(entity, bead_prefab.type);
    
    return entity;  
}

bool inspect(BeadPrefab* bead_prefab) {
    bool changed = false;
    ImGui::PathSelect<BeadPrefab>("File", &bead_prefab->file_path, true);

    changed |= inspect_dependencies(bead_prefab->dependencies, bead_prefab->file_path);
    
    changed |= ImGui::EnumCombo("Type", &bead_prefab->type);
    changed |= ImGui::PathSelect<ModelCPU>("Model", &bead_prefab->model_path, true);
    changed |= ImGui::DragFloat("Scale", &bead_prefab->scale, 0.01f);
    return changed;
}

Color bead_color(Bead bead) {
    switch (bead) {
        case (Bead_Oak): {
            return Color(0.82f, 0.66f, 0.37f);
        } break;
        case (Bead_Yew): {
            return Color(0.65f, 0.15f, 0.20f);
        } break;
        case (Bead_Amber): {
            return Color(0.87f, 0.37f, 0.02f);
        } break;
        case (Bead_Malachite): {
            return Color(0.24f, 0.80f, 0.60f);
        } break;
    }
    log_warning("Missing bead color");
    return palette::black;
}

bool inspect(DropChance::Entry* drop_chance_entry) {
    bool changed = false;
    changed |= ImGui::PathSelect<BeadPrefab>("Drop", &drop_chance_entry->bead_prefab_path);
    changed |= ImGui::SliderFloat("Drop Chance", &drop_chance_entry->drop_chance, 0.0f, 1.0f);
    return changed;
}

bool inspect(DropChance* drop_chance) {
    bool changed = false;
    if (ImGui::Button("  " ICON_FA_PLUS "  Add Drop  ")) {
        drop_chance->entries.emplace_back();
        changed = true;
    }
    for (auto it = drop_chance->entries.begin(); it != drop_chance->entries.end();) {
        ImGui::PushID(&*it);
        ImGui::BeginGroup();
        inspect(&*it);
        ImGui::EndGroup();
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TIMES)) {
            drop_chance->entries.remove_index(it - drop_chance->entries.begin(), false);
        } else {
            ++it;
        }
        ImGui::Separator();
        ImGui::PopID();
    }
    return changed;
    
}

}