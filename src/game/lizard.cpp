#include "lizard.hpp"

#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"
#include "game/input.hpp"
#include "editor/console.hpp"

namespace spellbook {

void lizard_system(Scene* scene) {
    auto& registry = scene->registry;
    
    auto lizards = registry.view<Lizard, LogicTransform>();
    auto enemies = registry.view<Health, LogicTransform, Traveler>();

    for (auto [entity, lizard, transform] : lizards.each()) {
        ZoneAttack* zone_attack = nullptr;

        if (zone_attack == nullptr)
            continue;
        
        // On cooldown
        if (scene->time <= zone_attack->last_tick + zone_attack->rate)
            continue;
        
        zone_attack->last_tick += zone_attack->rate; // Don't skip the deltatime
        for (auto [e_enemy, enemy_health, enemy_transform, _] : enemies.each()) {
            // Out of range
            if (math::length(transform.position - enemy_transform.position) > zone_attack->radius)
                continue;
            
            // Hit
            enemy_health.value -= zone_attack->damage;
        }
    }
}

void projectile_system(Scene* scene) {
    
}

entt::entity instance_prefab(Scene* scene, const LizardPrefab& lizard_prefab, v3i location) {
    static int i      = 0;
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "lizard", i++));
    
    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_model(lizard_prefab.model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f, 0.5f, 0.0f));
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type);

    switch (lizard_prefab.type) {
        default:
            console_error(fmt_("Unknown lizard type: {}", magic_enum::enum_name(lizard_prefab.type)), "game.lizard", ErrorType_Warning);
    }
    
    return entity;
}

void inspect(LizardPrefab* lizard_prefab) {
    ImGui::PathSelect("File", &lizard_prefab->file_path, "resources", FileType_Lizard);
    ImGui::EnumCombo("Type", &lizard_prefab->type);
    ImGui::PathSelect("Globe Model", &lizard_prefab->model_path, "resources", FileType_Model);
}

void save_lizard(const LizardPrefab& lizard_prefab) {
    auto j = from_jv<json>(to_jv(lizard_prefab));
    
    string ext = fs::path(lizard_prefab.file_path).extension().string();
    assert_else(ext == extension(FileType_Lizard));
    
    file_dump(j, to_resource_path(lizard_prefab.file_path).string());
}

LizardPrefab load_lizard(const string& input_path) {
    fs::path absolute_path = to_resource_path(input_path);
    check_else(fs::exists(absolute_path))
        return {};
    string ext = absolute_path.extension().string();
    assert_else(ext == extension(FileType_Lizard))
        return {};

    json j = parse_file(absolute_path.string());
    auto lizard_prefab = from_jv<LizardPrefab>(to_jv(j));
    lizard_prefab.file_path = absolute_path.string();
    return lizard_prefab;
}

}
