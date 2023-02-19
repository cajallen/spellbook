#include "lizard.hpp"

#include <corecrt_io.h>
#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "game.hpp"
#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"
#include "game/pose_controller.hpp"
#include "game/input.hpp"
#include "editor/console.hpp"
#include "general/matrix_math.hpp"

namespace spellbook {

void lizard_system(Scene* scene) {
    auto& registry = scene->registry;
    
    auto lizards = registry.view<Lizard, LogicTransform>();
    auto enemies = registry.view<Health, LogicTransform, Traveler>();
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
    auto& poser = scene->registry.emplace<PoseController>(entity, *model_comp.model_cpu.skeleton, PoseController::State_Invalid);
    scene->registry.emplace<Health>(entity, lizard_prefab.max_health, &scene->render_scene, lizard_prefab.hurt_path);
    scene->registry.emplace<Draggable>(entity);

    poser.set_state(PoseController::State_Idle, 0.0f);
    
    switch (lizard_prefab.type) {
        case (LizardType_Assassin): {
            liz.basic_ability = make_ability(scene, "Assassin Basic");
            liz.basic_ability->caster = entity;
            liz.basic_ability->pre_trigger_time = Stat(0.2f);
            liz.basic_ability->post_trigger_time = Stat(0.6f);
            liz.basic_ability->cooldown_time = Stat(0.8f);
            liz.basic_ability->start_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state(PoseController::State_Attacking, 0.1f, ability->pre_trigger_time.value() - 0.1f);
                }
            };
            liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
            liz.basic_ability->trigger_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto enemies = ability->scene->get_enemies(ability->target);
                for (auto& enemy : enemies) {
                    auto& health = ability->scene->registry.get<Health>(enemy);
                    auto& this_lt = ability->scene->registry.get<LogicTransform>(ability->caster);
                    auto& enemy_lt = ability->scene->registry.get<LogicTransform>(enemy);
                    health.damage(3.5f, enemy_lt.position - this_lt.position);
                }
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state(PoseController::State_Attacked, 0.1f, ability->post_trigger_time.value() - 0.1f);
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
        case (LizardType_Bulwark): {
            // TODO: spawn blood
            liz.basic_ability = make_ability(scene, "Bulwark Basic");
            liz.basic_ability->caster = entity;
            liz.basic_ability->pre_trigger_time = Stat(0.3f);
            liz.basic_ability->post_trigger_time = Stat(1.0f);
            liz.basic_ability->cooldown_time = Stat(1.0f);
            liz.basic_ability->start_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state(PoseController::State_Attacking, 0.1f, ability->pre_trigger_time.value() - 0.1f);
                }
            };
            liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
            liz.basic_ability->trigger_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto enemies = ability->scene->get_enemies(ability->target);
                for (auto& enemy : enemies) {
                    auto& health = ability->scene->registry.get<Health>(enemy);
                    auto& this_lt = ability->scene->registry.get<LogicTransform>(ability->caster);
                    auto& enemy_lt = ability->scene->registry.get<LogicTransform>(enemy);
                    health.damage(2.0f, enemy_lt.position - this_lt.position);
                }
                auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
                if (poser) {
                    poser->set_state(PoseController::State_Attacked, 0.1f, ability->post_trigger_time.value() - 0.1f);
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
            };
            liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
            liz.basic_ability->trigger_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
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
            };
            liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
            liz.basic_ability->trigger_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);

                quick_emitter(ability->scene, "Illusion Basic", v3(ability->target), "emitters/shadow_daggers.sbemt", 0.2f);
                struct IllusionBasicHurtPayload {
                    Scene* scene;
                    Lizard* lizard;
                    v3i target;
                };
                add_timer(ability->scene, "Illusion Basic Hurt Timer", [](Timer* timer, void* data) {
                    auto payload = (IllusionBasicHurtPayload*) data;
                    auto scene = payload->scene;
                    auto enemies = scene->get_enemies(payload->target);
                    for (auto& enemy : enemies) {
                        auto& health = scene->registry.get<Health>(enemy);
                        health.damage(2.5f, v3(0, 0, -1));
                    }
                    delete payload;
                },  new IllusionBasicHurtPayload{ability->scene,
                    &ability->scene->registry.get<Lizard>(ability->caster),
                    ability->target})
                    .start(0.3f);
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
        case (LizardType_Warlock): {
            liz.basic_ability = make_ability(scene, "Warlock Basic");
            liz.basic_ability->caster = entity;
            liz.basic_ability->pre_trigger_time = Stat(0.5f);
            liz.basic_ability->post_trigger_time = Stat(1.0f);
            liz.basic_ability->cooldown_time = Stat(1.2f);
            liz.basic_ability->start_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);

                add_tween_timer(ability->scene, "Emissive Up", [](Timer* timer, void* payload) {
                    auto ribbon_mat = game.renderer.get_material("models\\liz\\Ribbon.sbmat");
                    auto glyph_mat = game.renderer.get_material("models\\liz\\gylph.sbmat");
                    ribbon_mat->tints.emissive_tint.a = 1.0f - timer->remaining_time / timer->total_time;
                    glyph_mat->tints.emissive_tint.a = 1.0f - timer->remaining_time / timer->total_time;
                }, (void*) ability.id).start(ability->pre_trigger_time.value());
            };
            liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
            liz.basic_ability->trigger_callback = [](void* payload) {
                auto ability = id_ptr<Ability>((u64) payload);
                auto model = ability->scene->registry.try_get<Model>(ability->caster);
                auto transform = ability->scene->registry.try_get<ModelTransform>(ability->caster);
                auto& skeleton = model->model_cpu.skeleton;
                for (auto& bone : skeleton->bones) {
                    if (bone->name == "crystal") {
                        m44 t =  transform->get_transform() * model->model_cpu.root_node->cached_transform * bone->final_transform();
                        v3 pos = math::apply_transform(t, v3(0.0f, 0.5f, 0.0f));
                        quick_emitter(ability->scene, "Warlock Crush", v3(pos), "emitters/warlock_crush.sbemt", 0.1f);
                    }
                }
                add_tween_timer(ability->scene, "Emissive Down", [](Timer* timer, void* payload) {
                    auto ribbon_mat = game.renderer.get_material("models\\liz\\Ribbon.sbmat");
                    auto glyph_mat = game.renderer.get_material("models\\liz\\gylph.sbmat");
                    ribbon_mat->tints.emissive_tint.a = timer->remaining_time / timer->total_time;
                    glyph_mat->tints.emissive_tint.a = timer->remaining_time / timer->total_time;
                }, (void*) ability.id).start(ability->post_trigger_time.value());
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
                for (int x = -2; x <= 2; x++) {
                    for (int y = -2; y <= 2; y++) {
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
