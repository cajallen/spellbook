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
    model_comp.model_cpu = load_model(enemy_prefab.model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);

    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5));

    scene->registry.emplace<Traveler>(entity, vector<v3i>{}, enemy_prefab.max_speed);
    scene->registry.emplace<Health>(entity, enemy_prefab.max_health * 20.f, enemy_prefab.max_health * 20.f);
    
    return entity;  
}

void inspect(EnemyPrefab* enemy_prefab) {
    ImGui::PathSelect("File", &enemy_prefab->file_path, "resources", FileType_Enemy);
    ImGui::EnumCombo("Type", &enemy_prefab->type);
    ImGui::PathSelect("Model", &enemy_prefab->model_path, "resources", FileType_Model);
    ImGui::DragFloat("Max Health", &enemy_prefab->max_health, 0.01f, 0.0f);
    ImGui::DragFloat("Max Speed", &enemy_prefab->max_speed, 0.01f, 0.0f);
}

void save_enemy(const EnemyPrefab& enemy_prefab) {
    auto j = from_jv<json>(to_jv(enemy_prefab));
    
    string ext = fs::path(enemy_prefab.file_path).extension().string();
    assert_else(ext == extension(FileType_Enemy));
    
    file_dump(j, to_resource_path(enemy_prefab.file_path).string());
}

EnemyPrefab load_enemy(const string& input_path) {
    fs::path absolute_path = to_resource_path(input_path);
    check_else(fs::exists(absolute_path))
        return {};
    string ext = absolute_path.extension().string();
    assert_else(ext == extension(FileType_Enemy))
        return {};

    json j = parse_file(absolute_path.string());
    auto enemy_prefab = from_jv<EnemyPrefab>(to_jv(j));
    enemy_prefab.file_path = absolute_path.string();
    return enemy_prefab;
}


}
