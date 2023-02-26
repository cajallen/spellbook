#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entity/registry.hpp>

#include "general/string.hpp"
#include "general/vector.hpp"
#include "renderer/render_scene.hpp"
#include "game/camera_controller.hpp"
#include "game/timer.hpp"
#include "game/shop.hpp"
#include "game/player.hpp"


namespace spellbook {

struct LizardPrefab;
struct TilePrefab;
struct SpawnerPrefab;
struct ConsumerPrefab;
struct RoundInfo;

struct Scene {
    string           name;
    RenderScene      render_scene;
    Camera           camera;
    CameraController controller;
    entt::registry   registry;
    entt::entity     selected_entity;
    Shop shop;
    Player player;
    RoundInfo* round_info;
    
    bool edit_mode = true; // disables certain features
    float time = 0.0f;
    float delta_time = 0.0f;
    float time_scale = 1.0f;

    bool pause = false;

    plf::colony<Timer> timers;
    vector<TimerCallback> timer_callbacks;

    umap<v3i, entt::entity> visual_map_entities;
    
    void setup(const string& name);
    void update();
    void cleanup();

    void inspect_entity(entt::entity entity);
    void settings_window(bool* p_open);
    void output_window(bool* p_open);

    void model_cleanup(entt::registry&, entt::entity);
    void dragging_cleanup(entt::registry&, entt::entity);
    void health_cleanup(entt::registry&, entt::entity);
    void lizard_cleanup(entt::registry&, entt::entity);
    void enemy_cleanup(entt::registry&, entt::entity);

    void select_entity(entt::entity entity);

    // Helper query functions
    entt::entity get_lizard(v3i);
    entt::entity get_tile(v2i);
    entt::entity get_tile(v3i);
    entt::entity get_spawner(v3i);
    entt::entity get_consumer(v3i);
    vector<entt::entity> get_enemies(v3i);
    vector<entt::entity> get_any(v3i);

    entt::entity get(v3i, LizardPrefab* t);
    entt::entity get(v3i, TilePrefab* t);
    entt::entity get(v3i, SpawnerPrefab* t);
    entt::entity get(v3i, ConsumerPrefab* t);

    bool get_object_placement(v3i& pos);
    bool get_object_placement(v2i pixel_offset, v3i& pos);

    void set_edit_mode(bool to);
};

entt::entity quick_emitter(Scene* scene, const string& name, v3 position, const string& emitter_path, float duration);
entt::entity quick_emitter(Scene* scene, const string& name, v3 position, EmitterCPU emitter_cpu, float duration);

}
