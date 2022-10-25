#include "enemy.hpp"

#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "lib_ext/imgui_extra.hpp"

#include "components.hpp"
#include "scene.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const EnemyPrefab& enemy_prefab, v3i location) {
    static int i      = 0;
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "tower", i++));
    
    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_model(enemy_prefab.model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);

    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity, v3(location));

    scene->registry.emplace<Traveler>(entity, vector<v3i>{}, enemy_prefab.max_speed);
    scene->registry.emplace<Health>(entity, enemy_prefab.max_health, enemy_prefab.max_health);
    
    return entity;  
}

void inspect(EnemyPrefab* enemy_prefab) {
    EnumCombo("Type", &enemy_prefab->type);
    ImGui::InputText("Model", &enemy_prefab->model_path);
    ImGui::InputText("File", &enemy_prefab->file_path);
    ImGui::DragFloat("Max Health", &enemy_prefab->max_health, 0.01f, 0.0f);
    ImGui::DragFloat("Max Speed", &enemy_prefab->max_speed, 0.01f, 0.0f);
}

}
