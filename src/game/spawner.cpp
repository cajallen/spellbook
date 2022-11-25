#include "spawner.hpp"

#include "extension/imgui_extra.hpp"
#include "game/components.hpp"
#include "game/scene.hpp"
#include "game/input.hpp"
#include "game/game.hpp"

namespace spellbook {

std::function<EnemyPrefab*(f32* cost_left, f32* cooldown)> simple_select_enemy(EnemyPrefab* input_enemy_prefab, f32 input_cost, f32 input_cooldown) {
    return [input_enemy_prefab, input_cost, input_cooldown](f32* cost_left, f32* cooldown) -> EnemyPrefab* {
        if (*cost_left < input_cost)
            return nullptr;
        *cooldown += input_cooldown;
        *cost_left -= input_cost;
        return input_enemy_prefab;
    };
}

std::function<bool(f32 cost_total)> simple_wave(f32 input_threshold) {
    return [input_threshold](f32 cost_total) -> bool {
        return cost_total >= input_threshold;
    };
}

void spawner_system(Scene* scene) {
    for (auto [entity, spawner, logic_transform] : scene->registry.view<Spawner, LogicTransform>().each()) {
        spawner.cost_total += spawner.delta_cost.value() * Input::delta_time;
        spawner.cooldown -= Input::delta_time;
        
        if (spawner.wave_happening) {
            if (spawner.cooldown > 0)
                continue;
            auto enemy_prefab = spawner.select_enemy(&spawner.cost_total, &spawner.cooldown);
            if (enemy_prefab == nullptr) {
                spawner.wave_happening = false;
                continue;
            }
            
            instance_prefab(scene, *enemy_prefab, v3i(logic_transform.position));
        }
        
        
        if (!spawner.wave_happening) {
            // We don't want burst at start of next wave
            spawner.cooldown = math::max(spawner.cooldown, 0.0f);

            spawner.wave_happening = spawner.wave_start(spawner.cost_total);           
        }
    }
}

void inspect(SpawnerPrefab* spawner_prefab) {
    ImGui::PathSelect("file_path", &spawner_prefab->file_path, "resources", FileType_Spawner, true);

    ImGui::EnumCombo("enemy_selection", &spawner_prefab->enemy_selection);
    ImGui::PathSelect("enemy_path", &spawner_prefab->enemy_prefab_path, "resources", FileType_Enemy);
    ImGui::DragFloat("enemy_cost", &spawner_prefab->enemy_cost, 0.01f);
    ImGui::DragFloat("enemy_cooldown", &spawner_prefab->enemy_cooldown, 0.01f);

    ImGui::EnumCombo("wave_selection", &spawner_prefab->wave_selection);
    ImGui::DragFloat("wave_cost", &spawner_prefab->wave_cost, 0.01f);
    
    ImGui::DragFloat("delta_cost", &spawner_prefab->delta_cost, 0.01f);
}

void save_spawner(const SpawnerPrefab& spawner_prefab) {
    auto j = from_jv<json>(to_jv(spawner_prefab));
    
    string ext = fs::path(spawner_prefab.file_path).extension().string();
    assert_else(ext == extension(FileType_Spawner));
    
    file_dump(j, to_resource_path(spawner_prefab.file_path).string());
}

SpawnerPrefab load_spawner(const string& input_path) {
    fs::path absolute_path = to_resource_path(input_path);
    warn_else(fs::exists(absolute_path))
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
            spawner.select_enemy = simple_select_enemy(new EnemyPrefab(load_enemy(spawner_prefab.enemy_prefab_path)), spawner_prefab.enemy_cost, spawner_prefab.enemy_cooldown);
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
    model_comp.model_cpu = load_model(load_enemy(spawner_prefab.enemy_prefab_path).model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);
    
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f));

    return entity;
}

}
