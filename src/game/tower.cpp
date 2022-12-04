#include "tower.hpp"

#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"
#include "game/input.hpp"

namespace spellbook {

void tower_system(Scene* scene) {
    auto& registry = scene->registry;
    for (auto [entity, tower, transform] : registry.view<Tower, ModelTransform>().each()) {
        tower.current_rotation += tower.rotation_speed * Input::delta_time;
        transform.rotation.yaw = tower.current_rotation;

        auto& clouds_transform = registry.get<ModelTransform>(tower.clouds);
        clouds_transform.rotation.yaw = 0.6f * tower.current_rotation;
    }

    
    auto towers = registry.view<Tower, LogicTransform>();
    auto enemies = registry.view<Health, LogicTransform, Traveler>();

    for (auto [entity, tower, transform] : towers.each()) {
        ZoneAttack* zone_attack = nullptr;
        if (registry.any_of<Desert>(entity))
            zone_attack = &registry.get<Desert>(entity).attack;
        if (registry.any_of<Arctic>(entity))
            zone_attack = &registry.get<Arctic>(entity).attack;
        if (zone_attack == nullptr)
            continue;

        // On cooldown
        if (Input::time <= zone_attack->last_tick + zone_attack->rate)
            continue;
        
        zone_attack->last_tick += zone_attack->rate; // Don't skip the deltatime
        for (auto [e_enemy, enemy_health, enemy_transform, _] : enemies.each()) {
            // Out of range
            if (math::length(transform.position - enemy_transform.position) > zone_attack->radius)
                continue;
            
            // Hit
            enemy_health.value -= zone_attack->damage;
            
            if (registry.any_of<Arctic>(entity)) {
                Stat& speed = registry.get<Traveler>(e_enemy).max_speed;
                StatEffect& arctic_effect = registry.get<Arctic>(entity).slow;
                if (speed.effects.contains("arctic")) {
                    auto& effect = speed.effects["arctic"];
                    effect.until = Input::time + arctic_effect.until;
                    effect.stacks += 1;
                    effect.stacks = math::min(effect.stacks, effect.max_stacks);
                } else {
                    speed.effects["arctic"] = StatEffect {
                        .type = arctic_effect.type,
                        .value = arctic_effect.value,
                        .max_stacks = arctic_effect.max_stacks,
                        .until = Input::time + arctic_effect.until
                    };
                }
            }
        }
    }


    
}

void projectile_system(Scene* scene) {
    
}

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
        case (TowerType_Earth):
            break;
        case (TowerType_Archipelago):
            break;
        case (TowerType_Desert):
            break;
        case (TowerType_Energy):
            break;
        case (TowerType_Arctic): {
            auto& arctic = scene->registry.emplace<Arctic>(entity);
            arctic.attack.last_tick = Input::time;
        } break;
        case (TowerType_Lava):
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
    check_else(fs::exists(absolute_path))
        return {};
    string ext = absolute_path.extension().string();
    assert_else(ext == extension(FileType_Tower))
        return {};

    json j = parse_file(absolute_path.string());
    auto tower_prefab = from_jv<TowerPrefab>(to_jv(j));
    tower_prefab.file_path = absolute_path.string();
    return tower_prefab;
}

/*
void roller_system(Scene* scene) {
    ZoneScoped;
    auto rollers = scene->registry.view<Roller, ModelTransform>();

    static bool generated = false;
    if (!generated) {
        // dependencies
        game.renderer.upload_mesh(generate_icosphere(3));
        MaterialCPU material_cpu = {
            .file_path     = "roller_material",
            .color_tint    = palette::slate_gray
        };
        game.renderer.upload_material(material_cpu);
        generated = true;
    }

    for (auto [entity, roller, transform] : rollers.each()) {
        if (Input::time <= roller.last_tick + roller.rate)
            continue;
        roller.last_tick += roller.rate; // Don't skip the deltatime
        
        constexpr std::array<v2, 4> dirs { v2(-1, 0), v2( 0, 1), v2( 1, 0), v2( 0,-1) };

        for (int i = 0; i < 4; i++) {
            static int rollee_index = 0;
            auto       new_entity = scene->registry.create();
            
            scene->registry.emplace<Name>(new_entity, fmt_("rollee_{}", rollee_index));

            auto& model_comp = scene->registry.emplace<Model>(entity);
            model_comp.model_cpu = quick_model("Rollee", "icosphere_3", "roller_material");
            model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);

            scene->registry.emplace<LogicTransform>(new_entity, transform.translation + v3(0.2f * dirs[i], 0.0f));
            scene->registry.emplace<TransformLink>(new_entity, v3(0.5f));
            scene->registry.emplace<ModelTransform>(new_entity, v3(), euler(), roller.rollee_radius);
            
            scene->registry.emplace<Rollee>(new_entity, entity, roller.rollee_speed * v3(dirs[i], 0.0f), roller.rollee_lifetime);
            scene->registry.emplace<Collision>(new_entity, roller.rollee_radius);
            rollee_index++;
        }
    }
}

void rollee_system(Scene* scene) {
    ZoneScoped;
    
    for (auto [entity, rollee] : scene->registry.view<Rollee>().each()) {
        rollee.lifetime -= Input::delta_time;

        if (rollee.lifetime <= 0.0f) {
            scene->registry.emplace<Killed>(entity, Input::time);
            continue;
        }

        if (!scene->registry.valid(rollee.roller)) {
            scene->registry.emplace<Killed>(entity, Input::time);
            continue;
        }

        auto p_transform = scene->registry.try_get<ModelTransform>(entity);
        if (p_transform) {
            p_transform->translation += rollee.velocity * Input::delta_time;
        }

        auto p_collision = scene->registry.try_get<Collision>(entity);
        if (p_collision) {
            for (auto with : p_collision->with) {
                if (!scene->registry.valid(with))
                    continue;
                auto p_health = scene->registry.try_get<Health>(with);
                if (scene->registry.all_of<Traveler>(with)) {
                    p_health->value -= scene->registry.get<Roller>(rollee.roller).damage * Input::delta_time;
                }
            }
        }
        if (p_transform && p_collision) {
            p_transform->scale = math::mix(p_collision->radius,0.0f,math::smoothstep(0.2f, 0.0f, rollee.lifetime));
        }
    }
}*/


}
