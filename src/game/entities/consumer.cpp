#include "consumer.hpp"

#include <imgui.h>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "game/scene.hpp"
#include "game/game_file.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const ConsumerPrefab& consumer_prefab, v3i location) {
    static int i      = 0;
    
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "consumer", i++));

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(consumer_prefab.model_path));
    model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f, 0.5f, 0.0f));
    scene->registry.emplace<Consumer>(entity);
    
    return entity;
}

bool inspect(ConsumerPrefab* consumer_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &consumer_prefab->file_path, "resources/consumers", FileType_Consumer);
    changed |= ImGui::PathSelect("Model", &consumer_prefab->model_path, "resources/models", FileType_Model);
    return changed;
}

}
