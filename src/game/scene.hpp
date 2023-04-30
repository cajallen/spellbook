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
#include "game/map_targeting.hpp"


namespace spellbook {

struct LizardPrefab;
struct TilePrefab;
struct SpawnerPrefab;
struct ConsumerPrefab;
struct SpawnStateInfo;
namespace astar {
struct Navigation;
}

struct Scene {
    string           name;
    RenderScene      render_scene;
    Camera           camera;
    CameraController controller;
    entt::registry   registry;
    entt::entity     selected_entity;
    Shop shop;
    Player player;
    SpawnStateInfo* spawn_state_info;
    
    bool edit_mode = true; // disables certain features
    u32 frame = 0;
    float time = 0.0f;
    float delta_time = 0.0f;
    float time_scale = 1.0f;

    bool pause = false;

    plf::colony<Timer> timers;
    vector<TimerCallback> timer_callbacks;

    umap<v3i, entt::entity> visual_map_entities;

    std::unique_ptr<MapTargeting> targeting;
    std::unique_ptr<astar::Navigation> navigation;

    Scene();
    ~Scene();
    void setup(const string& name);
    void update();
    void cleanup();

    void inspect_entity(entt::entity entity);
    void settings_window(bool* p_open);
    void output_window(bool* p_open);

    void dragging_setup(entt::registry&, entt::entity);
    void model_cleanup(entt::registry&, entt::entity);
    void dragging_cleanup(entt::registry&, entt::entity);
    void health_cleanup(entt::registry&, entt::entity);
    void caster_cleanup(entt::registry&, entt::entity);
    void emitter_cleanup(entt::registry&, entt::entity);

    void select_entity(entt::entity entity);

    // Helper query functions
    bool is_casting_platform(v3i);
    entt::entity get_tile(v3i);
    entt::entity get_spawner(v3i);
    entt::entity get_consumer(v3i);
    vector<entt::entity> get_any(v3i);

    bool get_object_placement(v3i& pos);
    bool get_object_placement(v2i pixel_offset, v3i& pos);

    void set_edit_mode(bool to);
};

entt::entity quick_emitter(Scene* scene, const string& name, v3 position, const string& emitter_path, float duration);
entt::entity quick_emitter(Scene* scene, const string& name, v3 position, EmitterCPU emitter_cpu, float duration);

}
