#include "lizard.hpp"

#include <imgui.h>
#include <entt/entity/entity.hpp>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "editor/console.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizards/lizard_builder.hpp"


namespace spellbook {

void draw_lizard_dragging_preview(Scene* scene, entt::entity entity) {
    Lizard& lizard = scene->registry.get<Lizard>(entity);
    switch (lizard.type) {
        case LizardType_Champion:
            draw_champion_dragging_preview(scene, entity);
        break;
        default:
            console_error(fmt_("Unknown lizard type: {}", magic_enum::enum_name(lizard.type)), "game.lizard", ErrorType_Warning);
    }
}

entt::entity instance_prefab(Scene* scene, const LizardPrefab& lizard_prefab, v3i location) {
    static int i      = 0;
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "lizard", i++));
    
    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(lizard_prefab.model_path));
    model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f, 0.5f, 0.0f));
    scene->registry.emplace<Draggable>(entity);
    auto& poser = scene->registry.emplace<PoseController>(entity, *model_comp.model_cpu->skeleton);
    auto& health = scene->registry.emplace<Health>(entity, lizard_prefab.max_health, &scene->render_scene, lizard_prefab.hurt_path);

    poser.set_state(AnimationState_Idle, 0.0f);
    health.regen = Stat(lizard_prefab.health_regen);
    
    switch (lizard_prefab.type) {
        // case LizardType_Bulwark:
        //     build_bulwark(scene, entity, lizard_prefab);
        case LizardType_Barbarian:
            build_barbarian(scene, entity, lizard_prefab);
            break;
        case LizardType_Champion:
            build_champion(scene, entity, lizard_prefab);
            break;
        // case LizardType_Thief:
        //     build_thief(scene, entity, lizard_prefab);
        case LizardType_Assassin:
            build_assassin(scene, entity, lizard_prefab);
            break;
        case LizardType_Ranger:
            build_ranger(scene, entity, lizard_prefab);
            break;
        // case LizardType_SacrificeSupport:
        //     build_sacrifice_support(scene, entity, lizard_prefab);
        case LizardType_IllusionMage:
            build_illusion_mage(scene, entity, lizard_prefab);
            break;
        case LizardType_MindMage:
            build_mind_mage(scene, entity, lizard_prefab);
            break;
        case LizardType_WaterMage:
            build_water_mage(scene, entity, lizard_prefab);
            break;
        case LizardType_Warlock:
            build_warlock(scene, entity, lizard_prefab);
            break;
        default:
            console_error(fmt_("Unknown lizard type: {}", magic_enum::enum_name(lizard_prefab.type)), "game.lizard", ErrorType_Warning);
    }
    
    return entity;
}

bool inspect(LizardPrefab* lizard_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &lizard_prefab->file_path, "resources/lizards", FileType_Lizard);
    changed |= ImGui::EnumCombo("Type", &lizard_prefab->type);
    changed |= ImGui::PathSelect("Model", &lizard_prefab->model_path, "resources/models", FileType_Model);
    changed |= ImGui::DragFloat3("Default Direction", lizard_prefab->default_direction.data, 0.1f);
    changed |= ImGui::DragFloat("Health", &lizard_prefab->max_health, 0.5f, 0.5f, 0.0f, "%.1f");
    changed |= ImGui::DragFloat("Health Regen", &lizard_prefab->health_regen, 0.025f);
    return changed;
}

}
