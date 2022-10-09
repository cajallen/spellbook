#pragma once

#include <entt/entt.hpp>

#include "vector.hpp"
#include "string.hpp"
#include "slotmap.hpp"

#include "camera_controller.hpp"

#include "renderer/render_scene.hpp"

namespace spellbook {

struct Scene {
    string           name;
    RenderScene      render_scene;
    vector<Camera>   cameras;
    CameraController controller;
    entt::registry   registry;
    entt::entity     selected_entity;

    void setup();
    void update();
    void cleanup();

    void inspect_entity(entt::entity entity);
    void window(bool* p_open);

    void model_cleanup(entt::registry&, entt::entity);
    void dragging_cleanup(entt::registry&, entt::entity);
};

}
