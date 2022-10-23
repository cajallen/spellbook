#pragma once

#include <entt/entt.hpp>

#include "enemy.hpp"
#include "string.hpp"
#include "umap.hpp"

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
    v3i  position = v3i(0);
    bool path = false;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Traveler {
    vector<v3i> targets = {};
    float       velocity = math::random_f32(0.1f, 1.0f);

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Health {
    float value = 0.0f;
    StatEffect max = {};

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Spawner {
    float last_spawn = -FLT_MAX;
    float rate = FLT_MAX;

    EnemyPrefab enemy_prefab;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Consumer {
    f32 consume_distance = 0.01f;
    int amount_consumed = 0;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Killed {
    f32 when = -FLT_MAX;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Pyro {
    f32 radius = 0.0f;
    f32 last_tick = -FLT_MAX;
    f32 rate = FLT_MAX;
    f32 damage = 0.0f;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Roller {
    f32 last_tick = -FLT_MAX;
    f32 rate = FLT_MAX;
    f32 damage = 0.0f;
    f32 rollee_speed = 0.0f;
    f32 rollee_radius = 0.0f;
    f32 rollee_lifetime = 0.0f;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Rollee {
    entt::entity roller = {};
    v3           velocity = v3(0.0f);
    f32          lifetime = 0.0f;

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Dragging {
    f32 when = 0.0f;
    v3  start_position = v3(0.0f);
    v3  start_intersect = v3(0.0f);

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

struct Collision {
    f32                radius = 0.0f;
    uset<entt::entity> with = {};

    void inspect(Scene*);
    void preview_3d(Scene* scene, entt::entity entity);
};

}
