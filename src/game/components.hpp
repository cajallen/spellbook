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
    euler rotation = euler();
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
    f32 consume_distance = 0.02f;
    int amount_consumed = 0;
};

struct Killed {
    f32 when = -FLT_MAX;
};

struct Dragging {
    f32 start_time = 0.0f;
    v3  start_logic_position = v3(0.0f);
    v3  start_intersect = v3(0.0f);
    
    v3 target_position = v3(0.0f);
    v3 potential_logic_position = v3(0.0f);
};

struct Collision {
    f32                radius = 0.0f;
    uset<entt::entity> with = {};
};

struct PoseController {
    f32 time_scale = -1.0f;
    f32 time_to_target = 0.0f;
    f32 cycle_duration = 0.0f;
    string target_state;

    bool reset_time = false;

    void set_state(const string& new_state, float to_target, float cycle = 0.0f) {
        if (target_state != new_state) {
            reset_time = true;
            target_state = new_state;
            time_to_target = to_target;
            cycle_duration = cycle;
        }
    }
};

struct EmitterComponent {
    EmitterGPU* emitter;
    Timer* timer;
};

void inspect_components(Scene* scene, entt::entity entity);
void preview_3d_components(Scene* scene, entt::entity entity);

}
