#pragma once

#include <entt/fwd.hpp>
#include <imgui.h>

#include "visual_tile.hpp"
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
    v3    scale       = v3(1.0f);

    bool dirty = true;
    m44 transform;
    void set_translation(const v3& v);
    void set_rotation(const euler& e);
    void set_scale(const v3& v);
    const m44& get_transform();
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
    float buffer_value = 0.0f;
    
    Stat max_health = {};
    EmitterGPU* hurt_emitter;
    v3 hurt_direction;
    float hurt_until;

    Stat burn = {};

    Health(float health_value, RenderScene* render_scene, string hurt_emitter_path = "") {
        value = health_value;
        buffer_value = value;
        max_health = Stat(value);
        if (!hurt_emitter_path.empty())
            hurt_emitter = &instance_emitter(*render_scene, load_asset<EmitterCPU>(hurt_emitter_path, true));
    }

    void damage(float amount, v3 direction) {
        value -= amount;
        hurt_direction = math::normalize(direction);
        hurt_until = math::max(
            hurt_until,
            Input::time + math::smoothstep(0.0f, max_health.value(), amount) * 0.2f
        );
    }
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

struct VisualTileSetWidget {
    VisualTileSet* tile_set = nullptr;
};

void inspect_components(Scene* scene, entt::entity entity);
void preview_3d_components(Scene* scene, entt::entity entity);

}
