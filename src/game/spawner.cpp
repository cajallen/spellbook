#include "spawner.hpp"

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "extension/icons/font_awesome4.h"
#include "general/logger.hpp"
#include "game/components.hpp"
#include "game/scene.hpp"
#include "game/input.hpp"
#include "game/game.hpp"

namespace spellbook {

std::function<EnemySpawnInfo(int)> simple_select_enemy(EnemySpawnInfo enemy) {
    return [enemy](int id) -> EnemySpawnInfo {
        return enemy;
    };
}

std::function<bool(float cost_total)> simple_wave(float input_threshold) {
    return [input_threshold](float cost_total) -> bool {
        return cost_total >= input_threshold;
    };
}

void spawner_system(Scene* scene) {
    if (scene->edit_mode)
        return;
    
    for (auto [entity, spawner, logic_transform] : scene->registry.view<Spawner, LogicTransform>().each()) {
        spawner.cost_total += spawner.delta_cost.value() * Input::delta_time;
        spawner.cooldown -= Input::delta_time;
        
        if (spawner.wave_happening) {
            if (spawner.cost_total < 0.0f) {
                spawner.wave_happening = false;
                continue;
            }
            if (spawner.cooldown > -spawner.selected_enemy.spawn_pre_delay)
                continue;
            
            instance_prefab(scene, load_asset<EnemyPrefab>(spawner.selected_enemy.prefab_path), v3i(logic_transform.position));
            spawner.cost_total -= spawner.selected_enemy.cost;
            spawner.cooldown += spawner.selected_enemy.spawn_pre_delay + spawner.selected_enemy.spawn_post_delay;
            spawner.selected_enemy = spawner.select_enemy(spawner.wave_spawned++);
        }
        
        
        if (!spawner.wave_happening) {
            // We don't want burst at start of next wave
            spawner.cooldown = math::max(spawner.cooldown, 0.0f);

            spawner.wave_happening = spawner.wave_start(spawner.cost_total);
            spawner.selected_enemy = spawner.select_enemy(spawner.wave_spawned++);
        }
    }
}

bool inspect(EnemySpawnInfo* enemy_entry) {
    bool changed = false;
    changed |= ImGui::PathSelect("Path", &enemy_entry->prefab_path, "resources/enemies", FileType_Enemy);
    changed |= ImGui::DragFloat("Cost", &enemy_entry->cost, 0.01f);
    changed |= ImGui::DragFloat("Pre Delay", &enemy_entry->spawn_pre_delay, 0.01f);
    changed |= ImGui::DragFloat("Post Delay", &enemy_entry->spawn_post_delay, 0.01f);
    return changed;   
}

bool inspect(SpawnerPrefab* spawner_prefab) {
    bool changed = false;
    ImGui::PathSelect("file_path", &spawner_prefab->file_path, "resources/spawners", FileType_Spawner, true);

    changed |= ImGui::EnumCombo("enemy_selection", &spawner_prefab->enemy_selection);
    if (ImGui::Button(ICON_FA_PLUS, {100, 0})) {
        spawner_prefab->enemy_entries.emplace_back();
        changed = true;
    }
    for (auto& entry : spawner_prefab->enemy_entries) {
        ImGui::BeginGroup();
        changed |= inspect(&entry);
        ImGui::EndGroup();
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TIMES)) {
            spawner_prefab->enemy_entries.remove_index(spawner_prefab->enemy_entries.index(entry), true);
            break;
        } 
        ImGui::Separator();
    }

    changed |= ImGui::EnumCombo("wave_selection", &spawner_prefab->wave_selection);
    changed |= ImGui::DragFloat("wave_threshold", &spawner_prefab->wave_cost, 0.01f);
    changed |= ImGui::DragFloat("delta_cost", &spawner_prefab->delta_cost, 0.01f);
    return changed;
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

    json j = parse_file(absolute_path.string());
    auto spawner_prefab = from_jv<SpawnerPrefab>(to_jv(j));
    spawner_prefab.file_path = absolute_path.string();
    return spawner_prefab;
}


entt::entity instance_prefab(Scene* scene, const SpawnerPrefab& spawner_prefab, v3i location) {
    static int i      = 0;
    
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "spawner", i++));
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    
    Spawner spawner;
    spawner.delta_cost = spawner_prefab.delta_cost;
    switch (spawner_prefab.enemy_selection) {
        case (SpawnerPrefab::EnemySelection_Simple):
            auto entry = !spawner_prefab.enemy_entries.empty() ? spawner_prefab.enemy_entries.front() : EnemySpawnInfo{};
            spawner.select_enemy = simple_select_enemy(entry);
            break;
    }
    switch (spawner_prefab.wave_selection) {
        case (SpawnerPrefab::WaveSelection_Simple):
            spawner.wave_start = simple_wave(spawner_prefab.wave_cost);
            break;
    }
    spawner.cost_total = spawner_prefab.wave_cost - 1.f;
    scene->registry.emplace<Spawner>(entity, spawner);


    // Model
    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_asset<ModelCPU>(load_asset<EnemyPrefab>(spawner_prefab.enemy_entries.front().prefab_path).model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);
    
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f));

    return entity;
}

}
