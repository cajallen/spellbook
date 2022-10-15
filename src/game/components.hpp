#pragma once

#include <entt/entt.hpp>

#include "string.hpp"
#include "umap.hpp"

#include "geometry.hpp"
#include "math.hpp"

#include "renderer/assets/model.hpp"

namespace spellbook {

struct Scene;

// components
struct Name {
    string name;
};

struct Model {
    ModelCPU model_cpu;
    ModelGPU model_gpu;
    v3        offset = v3(0.0f);

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Transform {
    v3    translation = v3(0.0f);
    euler rotation    = euler();
    f32   scale       = 1.0f;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct GridSlot {
    v3i  position;
    bool path;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Traveler {
    vector<v3i> targets;
    float       velocity = math::random_f32(0.1f, 1.0f);

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Health {
    float value;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Spawner {
    float last_spawn;
    float rate;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Consumer {
    f32 consume_distance = 0.01f;
    int amount_consumed;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Killed {
    f32 when;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Pyro {
    f32 radius;
    f32 last_tick;
    f32 rate;
    f32 damage;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Roller {
    f32 last_tick;
    f32 rate;
    f32 damage;
    f32 rollee_speed;
    f32 rollee_radius;
    f32 rollee_lifetime;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Rollee {
    entt::entity roller;
    v3           velocity;
    f32          lifetime;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Dragging {
    f32 when;
    v3  start_position;
    v3  start_intersect;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Collision {
    f32                radius;
    uset<entt::entity> with;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

}
