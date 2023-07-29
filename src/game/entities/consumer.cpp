#include "consumer.hpp"

#include <imgui.h>

#include "enemy.hpp"
#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const ConsumerPrefab& consumer_prefab, v3i location) {
    
    // Make shrine
    entt::entity shrine_entity = scene->registry.create();
    entt::entity egg_entity = scene->registry.create();
    
    static int shrine_i = 0;
    scene->registry.emplace<Name>(shrine_entity, fmt_("{}_{}", consumer_prefab.shrine_model_path.stem(), shrine_i++));

    Model& shrine_model = scene->registry.emplace<Model>(shrine_entity);
    shrine_model.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(consumer_prefab.shrine_model_path));
    shrine_model.model_gpu = instance_model(scene->render_scene, *shrine_model.model_cpu);
    
    scene->registry.emplace<LogicTransform>(shrine_entity, v3(location));
    scene->registry.emplace<ModelTransform>(shrine_entity);
    scene->registry.emplace<TransformLink>(shrine_entity, v3(0.5f, 0.5f, 0.0f));
    scene->registry.emplace<Shrine>(shrine_entity, egg_entity, true);
    scene->registry.emplace<FloorOccupier>(shrine_entity);
    scene->registry.emplace<AddToInspect>(shrine_entity);

    
    Model& egg_model = scene->registry.emplace<Model>(egg_entity);
    egg_model.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(consumer_prefab.egg_model_path));
    egg_model.model_gpu = instance_model(scene->render_scene, *egg_model.model_cpu);

    static int egg_i = 0;
    scene->registry.emplace<Name>(egg_entity, fmt_("{}_{}", consumer_prefab.egg_model_path.stem(), egg_i++));
    scene->registry.emplace<LogicTransform>(egg_entity, v3(location));
    scene->registry.emplace<ModelTransform>(egg_entity);
    scene->registry.emplace<TransformLink>(egg_entity, v3(0.5f, 0.5f, 0.3f));
    scene->registry.emplace<Attachment>(egg_entity, shrine_entity, false);
    scene->registry.emplace<Egg>(egg_entity);
    scene->registry.emplace<Draggable>(egg_entity, 0.2f);
    scene->registry.emplace<AddToInspect>(egg_entity);
    scene->registry.emplace<Grounded>(egg_entity);
    
    return shrine_entity;
}

bool inspect(ConsumerPrefab* consumer_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &consumer_prefab->file_path, FileType_Consumer);
    changed |= ImGui::PathSelect("Shrine Model", &consumer_prefab->shrine_model_path, FileType_Model);
    changed |= ImGui::PathSelect("Egg Model", &consumer_prefab->egg_model_path, FileType_Model);
    return changed;
}

void consumer_system(Scene* scene) {
    for (auto [entity, shrine, shrine_logic_tfm] : scene->registry.view<Shrine, LogicTransform>().each()) {
        LogicTransform& egg_logic_tfm = scene->registry.get<LogicTransform>(shrine.egg_entity);
        if (!shrine.egg_attached) {
            Attachment& egg_attachment = scene->registry.get<Attachment>(shrine.egg_entity);
            if (!scene->registry.valid(egg_attachment.base)) {
                egg_logic_tfm.position = math::round(egg_logic_tfm.position);
            }
            continue;
        }

        egg_logic_tfm.position = shrine_logic_tfm.position;
        egg_logic_tfm.yaw += scene->delta_time * 1.0f;
    }
}


}
