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
    model_comp.model_cpu = load_model(tower_prefab.model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);

    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity, v3(location));
    
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
    EnumCombo("Type", &tower_prefab->type);
    ImGui::InputText("Model", &tower_prefab->model_path);
    ImGui::InputText("File", &tower_prefab->file_path);
}

}
