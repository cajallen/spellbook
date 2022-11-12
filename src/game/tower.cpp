#include "tower.hpp"

#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "lib_ext/imgui_extra.hpp"

#include "scene.hpp"
#include "components.hpp"
#include "input.hpp"


namespace spellbook {

entt::entity instance_prefab(Scene* scene, const TowerPrefab& tower_prefab, v3i location) {
    static int i      = 0;
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "tower", i++));
    
    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_model(tower_prefab.globe_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5));
    scene->registry.emplace<Tower>(entity, 1.0f);

    auto clouds_entity = scene->registry.create();

    auto& clouds_model_comp = scene->registry.emplace<Model>(clouds_entity);
    clouds_model_comp.model_cpu = load_model(tower_prefab.clouds_path);
    clouds_model_comp.model_gpu = instance_model(scene->render_scene, clouds_model_comp.model_cpu);
    scene->registry.emplace<LogicTransformAttach>(clouds_entity, entity);
    scene->registry.emplace<LogicTransform>(clouds_entity);
    scene->registry.emplace<ModelTransform>(clouds_entity);
    scene->registry.emplace<TransformLink>(clouds_entity, v3(0.5));

    scene->registry.get<Tower>(entity).clouds = clouds_entity;

    switch (tower_prefab.type) {
        case (TowerType_Roller):
            scene->registry.emplace<Roller>(entity, Input::time, 4.0f, 0.3f, 1.5f, 0.3f, 2.0f);
            break;
        case (TowerType_Pyro):
            scene->registry.emplace<Pyro>(entity, 2.0f, Input::time, 1.0f, 0.2f);
            break;
        default:
            console_error(fmt_("Unknown tower type: {}", magic_enum::enum_name(tower_prefab.type)), "game.tower", ErrorType_Warning);
    }
    
    return entity;
}

void inspect(TowerPrefab* tower_prefab) {
    ImGui::PathSelect("File", &tower_prefab->file_path, "resources", FileType_Tower);
    ImGui::EnumCombo("Type", &tower_prefab->type);
    ImGui::PathSelect("Globe Model", &tower_prefab->globe_path, "resources", FileType_Model);
    ImGui::PathSelect("Clouds Model", &tower_prefab->clouds_path, "resources", FileType_Model);
}

void save_tower(const TowerPrefab& tower_prefab) {
    auto j = from_jv<json>(to_jv(tower_prefab));
    
    string ext = fs::path(tower_prefab.file_path).extension().string();
    assert_else(ext == extension(FileType_Tower));
    
    file_dump(j, to_resource_path(tower_prefab.file_path).string());
}

TowerPrefab load_tower(const string& input_path) {
    fs::path absolute_path = to_resource_path(input_path);
    warn_else(fs::exists(absolute_path))
        return {};
    string ext = absolute_path.extension().string();
    assert_else(ext == extension(FileType_Tower))
        return {};

    json j = parse_file(absolute_path.string());
    auto tower_prefab = from_jv<TowerPrefab>(to_jv(j));
    tower_prefab.file_path = absolute_path.string();
    return tower_prefab;
}

}
