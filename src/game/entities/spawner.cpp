#include "spawner.hpp"

#include <entt/entity/entity.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "extension/icons/font_awesome4.h"
#include "general/logger.hpp"
#include "game/scene.hpp"
#include "game/input.hpp"
#include "game/game.hpp"
#include "game/entities/components.hpp"
#include "game/entities/enemy.hpp"

namespace spellbook {

void spawner_system(Scene* scene) {
    if (scene->edit_mode)
        return;

    for (auto [entity, spawner, logic_transform] : scene->registry.view<Spawner, LogicTransform>().each()) {
        if (!spawner.force_spawn_path.empty()) {
            instance_prefab(scene, load_asset<EnemyPrefab>(spawner.force_spawn_path), v3i(logic_transform.position));
            spawner.force_spawn_path = "";
        }
        
        // Advance Round ?
        while (spawner.state.round_number < spawner.level_state->round_number) {
            spawner.state.advance_round();
            spawner.cooldown = 0.0f;
        }

        // All rounds done, exit
        if (spawner.state.round_number >= spawner.spawn_info.rounds.size())
            continue;
        // Not started yet, exit
        if (spawner.state.round_number == -1)
            continue;

        spawner.cooldown -= scene->delta_time;
        
        // Advance Wave ?
        if (!spawner.spawn_wave) {
            RoundSpawnInfo& round_info = *spawner.spawn_info.rounds[spawner.state.round_number];
            // Check there's a wave to advance to
            if (spawner.state.wave_number + 1 < round_info.waves.size()) {
                // Not started yet, start
                if (spawner.state.wave_number == -1)
                    spawner.spawn_wave = true;
                // All enemies done, start
                if (!spawner.spawn_wave) {
                    WaveSpawnInfo& wave = *round_info.waves[spawner.state.wave_number];
                    if (spawner.state.enemy_number >= wave.enemies.size()) {
                        spawner.spawn_wave = true;
                        spawner.cooldown = wave.post_delay;
                    }
                }
            }
        }
        if (spawner.spawn_wave) {
            RoundSpawnInfo& round_info = *spawner.spawn_info.rounds[spawner.state.round_number];
            if (spawner.cooldown <= -round_info.waves[spawner.state.wave_number + 1]->pre_delay) {
                spawner.state.wave_number++;
                spawner.state.enemy_number = 0;
                spawner.spawn_wave = false;
            } else {
                continue;
            }
        }
        
        // Advance Enemy
        RoundSpawnInfo& round = *spawner.spawn_info.rounds[spawner.state.round_number];
        WaveSpawnInfo& wave = *round.waves[spawner.state.wave_number];
        // Check there's another enemy to spawn
        if (spawner.state.enemy_number >= wave.enemies.size())
            continue;
        EnemySpawnInfo& enemy_info = *wave.enemies[spawner.state.enemy_number];
        if (spawner.cooldown <= -enemy_info.pre_delay) {
            instance_prefab(scene, load_asset<EnemyPrefab>(enemy_info.enemy_prefab_path), v3i(logic_transform.position));
            spawner.cooldown = enemy_info.post_delay;
            spawner.state.enemy_number++;
        }
    }
}

bool inspect(EnemySpawnInfo* enemy_spawn_info) {
    bool changed = false;
    changed |= ImGui::PathSelect("Path", &enemy_spawn_info->enemy_prefab_path, "resources/enemies", FileType_Enemy);
    changed |= ImGui::DragFloat("Pre Delay", &enemy_spawn_info->pre_delay, 0.01f);
    changed |= ImGui::DragFloat("Post Delay", &enemy_spawn_info->post_delay, 0.01f);
    return changed;
}

bool inspect(WaveSpawnInfo* wave_info) {
    bool changed = false;
    changed |= ImGui::InputText("Name", &wave_info->name);
    changed |= ImGui::DragFloat("Pre Delay", &wave_info->pre_delay, 0.01f);
    changed |= ImGui::DragFloat("Post Delay", &wave_info->post_delay, 0.01f);
    if (ImGui::TreeNode("Enemies")) {
        changed |= ImGui::OrderedVector(wave_info->enemies,
            [](id_ptr<EnemySpawnInfo>& item) { ImGui::Text("%s", fs::path(item->enemy_prefab_path).stem().string().c_str()); return false; },
            [](vector<id_ptr<EnemySpawnInfo>>& values, bool pressed) {
                if (pressed) {
                    values.push_back(id_ptr<EnemySpawnInfo>::emplace());
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ENEMY_SPAWN_INFO")) {
                        assert_else(payload->DataSize == sizeof(uint64))
                            return;
                        uint64 id = *(const uint64*) payload->Data;
                        values.emplace_back(id);
                    }
                    ImGui::EndDragDropTarget();
                }
            },
            true
        );
        ImGui::TreePop();
    }
    return changed;
}

bool inspect(RoundSpawnInfo* round_info) {
    bool changed = false;
    changed |= ImGui::InputText("Name", &round_info->name);
    if (ImGui::TreeNode("Waves")) {
        changed |= ImGui::OrderedVector(round_info->waves,
            [](id_ptr<WaveSpawnInfo>& item) { ImGui::Text("%s", item->name.c_str()); return false; },
            [](vector<id_ptr<WaveSpawnInfo>>& values, bool pressed) {
                if (pressed) {
                    values.push_back(id_ptr<WaveSpawnInfo>::emplace());
                    values.back()->name = fmt_("{}", values.back().id);
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_WAVE_SPAWN_INFO")) {
                        assert_else(payload->DataSize == sizeof(uint64))
                            return;
                        uint64 id = *(const uint64*) payload->Data;
                        values.emplace_back(id);
                    }
                    ImGui::EndDragDropTarget();
                }
            },
            true
        );
        ImGui::TreePop();
    }
    return changed;
}

bool inspect(LevelSpawnInfo* level_spawn_info) {
    bool changed = false;
    changed |= ImGui::InputText("Name", &level_spawn_info->name);
    if (ImGui::TreeNode("Rounds")) {
        changed |= ImGui::OrderedVector(level_spawn_info->rounds,
            [](id_ptr<RoundSpawnInfo>& item) { ImGui::Text("%s", item->name.c_str()); return false; },
            [](vector<id_ptr<RoundSpawnInfo>>& values, bool pressed) {
                if (pressed) {
                    values.push_back(id_ptr<RoundSpawnInfo>::emplace());
                    values.back()->name = fmt_("{}", values.back().id);
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ROUND_SPAWN_INFO")) {
                        assert_else(payload->DataSize == sizeof(uint64))
                            return;
                        uint64 id = *(const uint64*) payload->Data;
                        values.emplace_back(id);
                    }
                    ImGui::EndDragDropTarget();
                }
            },
            true
        );
        ImGui::TreePop();
    }
    return changed;
}

bool inspect(SpawnerPrefab* spawner_prefab) {
    bool changed = false;
    
    ImGui::PathSelect("file_path", &spawner_prefab->file_path, "resources/spawners", FileType_Spawner, true);

    changed |= inspect_dependencies(spawner_prefab->dependencies, spawner_prefab->file_path);
    
    changed |= ImGui::PathSelect("Base Model", &spawner_prefab->model_file_path, "resources/models", FileType_Model);

    ImGui::Text("Level Spawn Info");
    changed |= inspect(&spawner_prefab->level_spawn_info);
    ImGui::Separator();
    ImGui::Text("Archive");

    ImGui::PushID("RoundsArchive");
    ImGui::Text("Rounds");
    ImGui::OrderedVector(spawner_prefab->rounds,
        [](id_ptr<RoundSpawnInfo>& item) {
            ImGui::BeginGroup();
            ImGui::PushID(item.id);
            bool changed = inspect(&*item);
            ImGui::PopID();
            ImGui::EndGroup();
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("DND_ROUND_SPAWN_INFO", &item.id, sizeof(uint64));
                ImGui::Text("Round");
                ImGui::EndDragDropSource();
            }
            return changed;
        },
        [](vector<id_ptr<RoundSpawnInfo>>& values, bool pressed) {
            if (pressed) {
                values.push_back(id_ptr<RoundSpawnInfo>::emplace());
                values.back()->name = fmt_("{}", values.back().id);
            }
        },
        true
    );
    ImGui::PopID();
    ImGui::Separator();

    ImGui::PushID("WavesArchive");
    ImGui::Text("Waves");
    ImGui::OrderedVector(spawner_prefab->waves,
        [](id_ptr<WaveSpawnInfo>& item) {
            ImGui::BeginGroup();
            ImGui::PushID(item.id);
            bool changed = inspect(&*item);
            ImGui::PopID();
            ImGui::EndGroup();
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("DND_WAVE_SPAWN_INFO", &item.id, sizeof(uint64));
                ImGui::Text("Wave");
                ImGui::EndDragDropSource();
            }
            return changed;
        },
        [](vector<id_ptr<WaveSpawnInfo>>& values, bool pressed) {
            if (pressed) {
                values.push_back(id_ptr<WaveSpawnInfo>::emplace());
                values.back()->name = fmt_("{}", values.back().id);
            }
        },
        true
    );
    ImGui::PopID();
    ImGui::Separator();

    ImGui::PushID("EnemiesArchive");
    ImGui::Text("Enemies");
    ImGui::OrderedVector(spawner_prefab->enemies,
        [](id_ptr<EnemySpawnInfo>& item) {
            ImGui::BeginGroup();
            ImGui::PushID(item.id);
            bool changed = inspect(&*item);
            ImGui::PopID();
            ImGui::EndGroup();
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("DND_ENEMY_SPAWN_INFO", &item.id, sizeof(uint64));
                ImGui::Text("Enemy");
                ImGui::EndDragDropSource();
            }
            return changed;
        },
        [](vector<id_ptr<EnemySpawnInfo>>& values, bool pressed) {
            if (pressed) {
                values.push_back(id_ptr<EnemySpawnInfo>::emplace());
            }
        },
        true
    );
    ImGui::PopID();
    return changed;
}

bool inspect(Spawner* spawner) {
    bool changed = false;
    ImGui::Text("Spawn Info");
    changed |= inspect(&spawner->spawn_info);
    ImGui::Text("Spawner Spawn State");
    changed |= inspect(&spawner->state);
    ImGui::Text("Level Spawn State");
    changed |= inspect(spawner->level_state);
    changed |= ImGui::DragFloat("Cooldown", &spawner->cooldown, 0.01f);

    changed |= ImGui::Checkbox("wave_spawning", &spawner->wave_spawning);
    changed |= ImGui::Checkbox("enemy_spawning", &spawner->enemy_spawning);
    changed |= ImGui::Checkbox("spawn_wave", &spawner->spawn_wave);
    changed |= ImGui::Checkbox("spawn_enemy", &spawner->spawn_enemy);

    ImGui::BeginTable("Spawner Debug Spawn Table", 1);
    ImGui::TableSetupColumn("Enemy");
    ImGui::TableHeadersRow();

    for (const auto& entry : fs::directory_iterator(to_resource_path("enemies"))) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        string button_name = fmt_("Spawn {}", entry.path().stem().string());
        if (ImGui::Button(button_name.c_str())) {
            spawner->force_spawn_path = entry.path().string();
        }
    }
    ImGui::EndTable();

    return changed;
}

bool inspect(SpawnStateInfo* spawn_state_info) {
    ImGui::Text("Round: %d,  Wave: %d,  Enemy: %d", spawn_state_info->round_number, spawn_state_info->wave_number, spawn_state_info->enemy_number);
    return false;
}


void save_spawner(const SpawnerPrefab& spawner_prefab) {
    auto j = from_jv<json>(to_jv(spawner_prefab));

    string ext = fs::path(spawner_prefab.file_path).extension().string();
    assert_else(ext == extension(FileType_Spawner));

    file_dump(j, to_resource_path(spawner_prefab.file_path).string());
}

SpawnerPrefab load_spawner(const string& input_path) {
    fs::path absolute_path = to_resource_path(input_path);
    check_else(fs::exists(absolute_path))
        return {};
    string ext = absolute_path.extension().string();
    assert_else(ext == extension(FileType_Spawner))
        return {};

    json j                   = parse_file(absolute_path.string());
    auto spawner_prefab      = from_jv<SpawnerPrefab>(to_jv(j));
    spawner_prefab.file_path = absolute_path.string();
    return spawner_prefab;
}


entt::entity instance_prefab(Scene* scene, const SpawnerPrefab& spawner_prefab, v3i location) {
    static int i = 0;

    auto entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "spawner", i++));
    scene->registry.emplace<AddToInspect>(entity);
    scene->registry.emplace<LogicTransform>(entity, v3(location));

    if (!spawner_prefab.model_file_path.empty()) {
        auto& model_comp = scene->registry.emplace<Model>(entity);
        model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(spawner_prefab.model_file_path));
        model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);
        
        scene->registry.emplace<ModelTransform>(entity);
        scene->registry.emplace<TransformLink>(entity, v3(0.5f, 0.5f, 0.0f));
    }
    
    scene->registry.emplace<Spawner>(entity, spawner_prefab.level_spawn_info, SpawnStateInfo{}, scene->spawn_state_info);
    scene->registry.emplace<FloorOccupier>(entity);
    // // Model
    // if (!spawner_prefab.enemies.empty()) {
    //     auto& model_comp     = scene->registry.emplace<Model>(entity);
    //     model_comp.model_cpu = std::make_unique<ModelCPU>(
    //         load_asset<ModelCPU>(load_asset<EnemyPrefab>(spawner_prefab.enemies.front()->enemy_prefab_path).model_path));
    //     model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);
    //     
    //     scene->registry.emplace<ModelTransform>(entity);
    //     scene->registry.emplace<TransformLink>(entity, v3(0.5f));
    // }

    // Preload all of the enemies
    for (auto& enemy_info : spawner_prefab.enemies) {
        load_asset<ModelCPU>(load_asset<EnemyPrefab>(enemy_info->enemy_prefab_path).base_model_path);
        load_asset<ModelCPU>(load_asset<EnemyPrefab>(enemy_info->enemy_prefab_path).attachment_model_path);
    }

    return entity;
}

SpawnerPrefab from_jv_impl(const json_value& jv, SpawnerPrefab* _) {
    json j = from_jv<json>(jv);
    SpawnerPrefab value;

    if (j.contains("rounds")) {
        for (const json_value& jv : j["rounds"]->get_list()) {
            id_ptr<RoundSpawnInfo> node = from_jv_impl(jv, (id_ptr<RoundSpawnInfo>*) 0);
            value.rounds.push_back(node);
        }
    }
    if (j.contains("waves")) {
        for (const json_value& jv : j["waves"]->get_list()) {
            id_ptr<WaveSpawnInfo> node = from_jv_impl(jv, (id_ptr<WaveSpawnInfo>*) 0);
            value.waves.push_back(node);
        }
    }
    if (j.contains("enemies")) {
        for (const json_value& jv : j["enemies"]->get_list()) {
            id_ptr<EnemySpawnInfo> node = from_jv_impl(jv, (id_ptr<EnemySpawnInfo>*) 0);
            value.enemies.push_back(node);
        }
    }

    FROM_JSON_ELE(level_spawn_info);
    FROM_JSON_ELE(model_file_path);
    
    return value;
}

inline json_value to_jv(const SpawnerPrefab& value) {
    auto j = json();

    vector<json_value> rounds;
    for (id_ptr<RoundSpawnInfo> node : value.rounds) {
        rounds.push_back(to_jv_full(node));
    }
    j["rounds"] = make_shared<json_value>(to_jv(rounds));
    
    vector<json_value> waves;
    for (id_ptr<WaveSpawnInfo> node : value.waves) {
        waves.push_back(to_jv_full(node));
    }
    j["waves"] = make_shared<json_value>(to_jv(waves));

    vector<json_value> enemies;
    for (id_ptr<EnemySpawnInfo> node : value.enemies) {
        enemies.push_back(to_jv_full(node));
    }
    j["enemies"] = make_shared<json_value>(to_jv(enemies));
    
    TO_JSON_ELE(level_spawn_info);
    TO_JSON_ELE(model_file_path);
    
    return to_jv(j);
}

}
