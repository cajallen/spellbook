#include "lizard.hpp"

#include <corecrt_io.h>
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
    model_comp.model_cpu = load_asset<ModelCPU>(lizard_prefab.model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f, 0.5f, 0.0f));
    auto& liz = scene->registry.emplace<Lizard>(entity, lizard_prefab.type);
    scene->registry.emplace<PoseController>(entity, 1.0f, 0.0f, 2.0f, "default");

    switch (lizard_prefab.type) {
        case (LizardType_Assassin): {
            // TODO: spawn blood
            liz.basic_ability = make_ability(scene, "Assassin Basic");
            liz.basic_ability->caster = entity;
            liz.basic_ability->pre_trigger_time = Stat(0.2f);
            liz.basic_ability->post_trigger_time = Stat(0.6f);
            liz.basic_ability->cooldown_time = Stat(0.8f);
            liz.basic_ability->start_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state("attacking", ability->pre_trigger_time.value());
                }
            };
            liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
            liz.basic_ability->trigger_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto enemies = ability->scene->get_enemies(ability->target);
                for (auto& enemy : enemies) {
                    auto& health = ability->scene->registry.get<Health>(enemy);
                    health.value -= 1.0f;
                }
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state("default", ability->post_trigger_time.value(), 2.0f);
                }
            };
            liz.basic_ability->trigger_payload = (void*) liz.basic_ability.id;

            liz.basic_ability->targeting_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                v3i caster_pos = math::round_cast(ability->scene->registry.get<LogicTransform>(ability->caster).position);
                struct Entry {
                    v3i offset = {};
                    int count;
                };
                vector<Entry> entries;
                auto add_entry = [&ability, &entries, &caster_pos](v3i offset) {
                    entries.emplace_back(offset, ability->scene->get_enemies(caster_pos + offset).size());
                };
                add_entry(v3i( 1, 0,0));
                add_entry(v3i( 0, 1,0));
                add_entry(v3i(-1, 0,0));
                add_entry(v3i( 0,-1,0));
                vector closest_entries = {entries.front()};
                for (auto& entry : entries) {
                    if (entry.count > closest_entries.begin()->count)
                        closest_entries = {entry};
                    else if (entry.count == closest_entries.begin()->count)
                        closest_entries.push_back(entry);
                }
                if (closest_entries.front().count > 0) {
                    ability->target = caster_pos + closest_entries[math::random_s32(closest_entries.size())].offset;
                    ability->has_target = true;
                } else {
                    ability->has_target = false;
                }
            };
            liz.basic_ability->targeting_payload = (void*) liz.basic_ability.id;
        } break;
        case (LizardType_StormMage): {
            liz.basic_ability = make_ability(scene, "Storm Basic");
            liz.basic_ability->caster = entity;
            liz.basic_ability->pre_trigger_time = Stat(0.4f);
            liz.basic_ability->post_trigger_time = Stat(0.4f);
            liz.basic_ability->cooldown_time = Stat(1.0f);
            liz.basic_ability->start_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state("attacking", ability->pre_trigger_time.value());
                }
            };
            liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
            liz.basic_ability->trigger_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state("default", ability->post_trigger_time.value(), 2.0f);
                }
            };
            liz.basic_ability->trigger_payload = (void*) liz.basic_ability.id;

            liz.basic_ability->targeting_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                ability->has_target = false;
            };
            liz.basic_ability->targeting_payload = (void*) liz.basic_ability.id;
        } break;
        case (LizardType_IllusionMage): {
            liz.basic_ability = make_ability(scene, "Illusion Basic");
            // TODO: spawn thing, cast every now and then, dagger spawners stack
            liz.basic_ability->caster = entity;
            liz.basic_ability->pre_trigger_time = Stat(0.8f);
            liz.basic_ability->post_trigger_time = Stat(0.3f);
            liz.basic_ability->cooldown_time = Stat(0.3f);
            liz.basic_ability->start_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state("attacking", ability->pre_trigger_time.value());
                }
            };
            liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
            liz.basic_ability->trigger_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state("default", ability->post_trigger_time.value(), 2.0f);
                }

                quick_emitter(ability->scene, "Illusion Basic Emitter", v3(ability->target), "emitters/shadow_daggers.sbemt", 0.2f);
                struct IllusionBasicHurtPayload {
                    Scene* scene;
                    v3i target;
                };
                add_timer(ability->scene, "Illusion Basic Hurt Timer", [](void* data) {
                    auto payload = (IllusionBasicHurtPayload*) data;
                    auto enemies = payload->scene->get_enemies(payload->target);
                    for (auto& enemy : enemies) {
                        auto& health = payload->scene->registry.get<Health>(enemy);
                        health.value -= 1.0f;
                    }
                    delete payload;
                }, new IllusionBasicHurtPayload{ability->scene, ability->target}).start(0.3f);
            };
            liz.basic_ability->trigger_payload = (void*) liz.basic_ability.id;

            liz.basic_ability->targeting_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                v3i caster_pos = math::round_cast(ability->scene->registry.get<LogicTransform>(ability->caster).position);
                struct Entry {
                    v3i offset = {};
                    int count;
                };
                vector<Entry> entries;
                auto add_entry = [&ability, &entries, &caster_pos](v3i offset) {
                    entries.emplace_back(offset, ability->scene->get_enemies(caster_pos + offset).size());
                };
                for (int x = -3; x <= 3; x++) {
                    for (int y = -3; y <= 3; y++) {
                        add_entry(v3i(x,y,0));
                    }
                }
                vector closest_entries = {entries.front()};
                for (auto& entry : entries) {
                    if (entry.count > closest_entries.begin()->count)
                        closest_entries = {entry};
                    else if (entry.count == closest_entries.begin()->count)
                        closest_entries.push_back(entry);
                }
                if (closest_entries.front().count > 0) {
                    ability->target = caster_pos + closest_entries[math::random_s32(closest_entries.size())].offset;
                    ability->has_target = true;
                } else {
                    ability->has_target = false;
                }
            };
            liz.basic_ability->targeting_payload = (void*) liz.basic_ability.id;
        } break;
        default:
            console_error(fmt_("Unknown lizard type: {}", magic_enum::enum_name(lizard_prefab.type)), "game.lizard", ErrorType_Warning);
    }
    
    return entity;
}

bool inspect(LizardPrefab* lizard_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &lizard_prefab->file_path, "resources", FileType_Lizard);
    changed |= ImGui::EnumCombo("Type", &lizard_prefab->type);
    changed |= ImGui::PathSelect("Model", &lizard_prefab->model_path, "resources", FileType_Model);
    return changed;
}

}
