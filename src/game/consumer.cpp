#include "consumer.hpp"

#include <entt/entt.hpp>
#include <imgui.h>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"
#include "game/game_file.hpp"
#include "editor/asset_browser.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const ConsumerPrefab& consumer_prefab, v3i location) {
    static int i      = 0;
    
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "consumer", i++));

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_asset<ModelCPU>(consumer_prefab.model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<PoseController>(entity, 1.0f, 0.0f, 4.0f, "default");
    scene->registry.emplace<TransformLink>(entity, v3(0.5f));
    scene->registry.emplace<Consumer>(entity);

    return entity;
}

bool inspect(ConsumerPrefab* consumer_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &consumer_prefab->file_path, "resources", FileType_Consumer, true);
    changed |= ImGui::PathSelect("Model", &consumer_prefab->model_path, "resources", FileType_Model, true);
    return changed;
}

}
