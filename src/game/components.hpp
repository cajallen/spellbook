#pragma once

#include <entt/fwd.hpp>
#include <imgui.h>

#include "general/string.hpp"
#include "general/umap.hpp"
#include "general/geometry.hpp"
#include "game/enemy.hpp"
#include "game/scene.hpp"
#include "game/stat.hpp"
#include "renderer/assets/model.hpp"

namespace spellbook {

struct Scene;

// TODO: anim controller 

struct Name {
    string name;
};

struct Model {
    ModelCPU model_cpu = {};
    ModelGPU model_gpu = {};
};

struct LogicTransform {
    v3 position = v3(0.0f);
};

struct ModelTransform {
    v3    translation = v3(0.0f);
    euler rotation    = euler();
    f32   scale       = 1.0f;
};

struct TransformLink {
    v3 offset = v3(0.5f);
};

struct LogicTransformAttach {
    entt::entity to = entt::null;
    v3 offset = v3(0.0f);
};

struct GridSlot {
    bool path = false;
};

struct Traveler {
    vector<v3i> targets = {};
    Stat max_speed = {};
};

struct Health {
    float value = 0.0f;
    Stat max_health = {};

    Stat burn = 0.f;
};

struct Consumer {
    f32 consume_distance = 0.01f;
    int amount_consumed = 0;
};

struct Killed {
    f32 when = -FLT_MAX;
};

struct Dragging {
    f32 when = 0.0f;
    v3  start_logic_position = v3(0.0f);
    v3  start_intersect = v3(0.0f);
    
    f32 vertical_offset = 0.0f;
    v3 logic_position = v3(0.0f);
};

struct Collision {
    f32                radius = 0.0f;
    uset<entt::entity> with = {};
};

void inspect_components(Scene* scene, entt::entity entity);
void preview_3d_components(Scene* scene, entt::entity entity);

}
