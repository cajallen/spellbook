#pragma once

#include <entt/entt.hpp>

#include "general/vector.hpp"
#include "general/string.hpp"

#include "camera_controller.hpp"
#include "game/consumer.hpp"
#include "game/spawner.hpp"
#include "game/tile.hpp"
#include "game/lizard.hpp"

#include "renderer/render_scene.hpp"

namespace spellbook {

struct Scene {
    string           name;
    RenderScene      render_scene;
    Camera           camera;
    CameraController controller;
    entt::registry   registry;
    entt::entity     selected_entity;

    bool edit_mode = true; // disables certain features
    float time = 0.0f;
    float delta_time = 0.0f;
    float time_scale = 0.0f;
    
    void setup(const string& name);
    void update();
    void cleanup();

    void inspect_entity(entt::entity entity);
    void settings_window(bool* p_open);
    void output_window(bool* p_open);

    void model_cleanup(entt::registry&, entt::entity);
    void dragging_cleanup(entt::registry&, entt::entity);

    
    entt::entity get_lizard(v3i);
    entt::entity get_tile(v3i);
    entt::entity get_spawner(v3i);
    entt::entity get_consumer(v3i);
    vector<entt::entity> get_enemies(v3i);
    vector<entt::entity> get_any(v3i);

    entt::entity get(v3i, LizardPrefab* t);
    entt::entity get(v3i, TilePrefab* t);
    entt::entity get(v3i, SpawnerPrefab* t);
    entt::entity get(v3i, ConsumerPrefab* t);
};

}
