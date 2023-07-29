#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entity/registry.hpp>

#include "general/navigation_path.hpp"
#include "general/bitmask_3d.hpp"
#include "renderer/render_scene.hpp"
#include "game/camera_controller.hpp"
#include "game/timer.hpp"
#include "game/shop.hpp"
#include "game/player.hpp"
#include "game/map_targeting.hpp"
#include "game/audio.hpp"


namespace spellbook {

struct LizardPrefab;
struct TilePrefab;
struct SpawnerPrefab;
struct ConsumerPrefab;
struct SpawnStateInfo;
namespace astar {
struct Navigation;
}

struct PathInfo {
    entt::entity spawner;
    entt::entity consumer;
    Path path;
};

struct MapData {
    bool dirty = false;
    Bitmask3D solids;
    Bitmask3D path_solids;
    Bitmask3D slot_solids;
    Bitmask3D unstandable_solids;
    umap<v3i, Direction> ramps;

    void update(entt::registry& registry);
    void clear();
};

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
    Audio          audio;
    
    bool edit_mode = true; // disables certain features
    uint32 frame = 0;
    float time = 0.0f;
    float delta_time = 0.0f;
    float time_scale = 1.0f;

    bool pause = false;

    TimerManager timer_manager;

    umap<v3i, entt::entity> visual_map_entities;
    MapData map_data;
    std::unique_ptr<MapTargeting> targeting;
    
    std::unique_ptr<astar::Navigation> navigation;
    vector<PathInfo> paths;

    Scene();
    ~Scene();
    void setup(const string& name);
    void update();
    void cleanup();

    void inspect_entity(entt::entity entity);
    void settings_window(bool* p_open);
    void output_window(bool* p_open);
    void select_entity(entt::entity entity);

    // Helper query functions
    bool is_casting_platform(v3i);
    entt::entity get_tile(v3i);
    entt::entity get_spawner(v3i);
    entt::entity get_shrine(v3i);
    vector<entt::entity> get_any(v3i);

    bool get_object_placement(v3i& pos);
    bool get_object_placement(v2i pixel_offset, v3i& pos);

    void set_edit_mode(bool to);
};

void update_paths(vector<PathInfo>& paths, Scene& scene);
void render_paths(vector<PathInfo>& paths, RenderScene& scene, float time);

entt::entity quick_emitter(Scene* scene, const string& name, v3 position, const FilePath& emitter_path, float duration);
entt::entity quick_emitter(Scene* scene, const string& name, v3 position, EmitterCPU emitter_cpu, float duration);

}
