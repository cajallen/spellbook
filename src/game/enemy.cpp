#include "enemy.hpp"

#include <entt/entt.hpp>
#include <imgui.h>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/components.hpp"
#include "game/scene.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const EnemyPrefab& enemy_prefab, v3i location) {
    static int i      = 0;
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "enemy", i++));
    
    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_asset<ModelCPU>(enemy_prefab.model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);

    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5));

    scene->registry.emplace<Traveler>(entity, vector<v3i>{}, enemy_prefab.max_speed);
    scene->registry.emplace<Health>(entity, enemy_prefab.max_health, enemy_prefab.max_health);
    
    return entity;  
}

bool inspect(EnemyPrefab* enemy_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &enemy_prefab->file_path, "resources", FileType_Enemy);
    changed |= ImGui::EnumCombo("Type", &enemy_prefab->type);
    changed |= ImGui::PathSelect("Model", &enemy_prefab->model_path, "resources", FileType_Model);
    changed |= ImGui::DragFloat("Max Health", &enemy_prefab->max_health, 0.01f, 0.0f);
    changed |= ImGui::DragFloat("Max Speed", &enemy_prefab->max_speed, 0.01f, 0.0f);
    return changed;
}



}
