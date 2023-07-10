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
#include "game/entities/caster.hpp"
#include "game/entities/lizards/lizard_builder.hpp"


namespace spellbook {

void draw_lizard_dragging_preview(Scene* scene, entt::entity entity) {
    Lizard& lizard = scene->registry.get<Lizard>(entity);
    switch (lizard.type) {
        case LizardType_Champion:
            draw_champion_dragging_preview(scene, entity);
            break;
        case LizardType_Warlock:
            draw_warlock_dragging_preview(scene, entity);
            break;
        case LizardType_Assassin:
            draw_assassin_dragging_preview(scene, entity);
            break;
        case LizardType_Ranger:
            draw_ranger_dragging_preview(scene, entity);
        break;
        default:
            console_error(fmt_("Unknown lizard type: {}", magic_enum::enum_name(lizard.type)), "game.lizard", ErrorType_Warning);
    }
}

entt::entity instance_prefab(Scene* scene, const LizardPrefab& lizard_prefab, v3i location) {
    static int i      = 0;
    auto       entity = setup_basic_unit(scene, lizard_prefab.model_path, v3(location), lizard_prefab.max_health, lizard_prefab.hurt_path);

    static int lizard_i = 0;
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", fs::path(lizard_prefab.file_path).stem().string(), lizard_i++));
    
    scene->registry.emplace<Caster>(entity, scene);
    scene->registry.emplace<Draggable>(entity, 0.5f);
    
    ModelTransform& model_tfm = scene->registry.get<ModelTransform>(entity);
    model_tfm.set_scale(v3(lizard_prefab.scale));
    
    PoseController* poser = scene->registry.try_get<PoseController>(entity);
    if (poser)
        poser->set_state(AnimationState_Idle, 0.0f);
    Health& health = scene->registry.get<Health>(entity);
    health.regen = std::make_unique<Stat>(scene, lizard_prefab.health_regen);
    
    switch (lizard_prefab.type) {
        case LizardType_Barbarian:
            build_barbarian(scene, entity, lizard_prefab);
            break;
        case LizardType_Champion:
            build_champion(scene, entity, lizard_prefab);
            break;
        case LizardType_Assassin:
            build_assassin(scene, entity, lizard_prefab);
            break;
        case LizardType_Ranger:
            build_ranger(scene, entity, lizard_prefab);
            break;
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

    changed |= inspect_dependencies(lizard_prefab->dependencies, lizard_prefab->file_path);
    
    changed |= ImGui::EnumCombo("Type", &lizard_prefab->type);
    changed |= ImGui::PathSelect("Model", &lizard_prefab->model_path, "resources/models", FileType_Model);
    changed |= ImGui::DragFloat3("Default Direction", lizard_prefab->default_direction.data, 0.1f);
    changed |= ImGui::DragFloat("Health", &lizard_prefab->max_health, 0.5f, 0.5f, 0.0f, "%.1f");
    changed |= ImGui::DragFloat("Health Regen", &lizard_prefab->health_regen, 0.02f);
    changed |= ImGui::DragFloat("Scale", &lizard_prefab->scale, 0.02f);
    return changed;
}

}