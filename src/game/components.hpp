#pragma once

#include <entt/entt.hpp>
#include <imgui.h>

#include "enemy.hpp"
#include "string.hpp"
#include "umap.hpp"
#include "scene.hpp"

#include "geometry.hpp"
#include "math.hpp"
#include "stat.hpp"

#include "renderer/assets/model.hpp"

namespace spellbook {

struct Scene;

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
};

struct Consumer {
    f32 consume_distance = 0.01f;
    int amount_consumed = 0;
};

struct Killed {
    f32 when = -FLT_MAX;
};

struct Tower {
    f32 rotation_speed = 1.0f;
    f32 current_rotation = 0.0f;

    entt::entity clouds;
};

struct Pyro {
    f32 radius = 0.0f;
    f32 last_tick = -FLT_MAX;
    f32 rate = FLT_MAX;
    f32 damage = 0.0f;
};

struct Roller {
    f32 last_tick = -FLT_MAX;
    f32 rate = FLT_MAX;
    f32 damage = 0.0f;
    f32 rollee_speed = 0.0f;
    f32 rollee_radius = 0.0f;
    f32 rollee_lifetime = 0.0f;
};

struct Rollee {
    entt::entity roller = {};
    v3           velocity = v3(0.0f);
    f32          lifetime = 0.0f;
};

struct Dragging {
    f32 when = 0.0f;
    v3  start_position = v3(0.0f);
    v3  start_intersect = v3(0.0f);
};

struct Collision {
    f32                radius = 0.0f;
    uset<entt::entity> with = {};
};

void inspect_components(Scene* scene, entt::entity entity);
void preview_3d_components(Scene* scene, entt::entity entity);

}
