#pragma once

#include <entt/entity/entity.hpp>

#include "general/string.hpp"
#include "general/umap.hpp"
#include "general/geometry.hpp"
#include "game/entities/stat.hpp"
#include "renderer/assets/model.hpp"

namespace spellbook {

struct Scene;
struct EmitterGPU;
struct Timer;

struct Name {
    string name;
};

struct Model {
    std::unique_ptr<ModelCPU> model_cpu = nullptr;
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
    float rotate_speed = 2.0f;
};

struct GridSlot {
    bool path = false;
    bool ramp = false;
    Direction direction = Direction_Up;
};

struct Health {
    float value = 0.0f;
    float buffer_value = 0.0f;
    
    Stat max_health = {};
    EmitterGPU* hurt_emitter = nullptr;
    v3 hurt_direction = {};
    float hurt_until = 0.0f;

    Stat burn = {};
    Stat regen = {};

    Health(float health_value, RenderScene* render_scene, const string& hurt_emitter_path = "");
    void damage(float amount, v3 direction);
};

struct Killed {
    bool drop = false;
    f32 when = -FLT_MAX;
};

struct Draggable {
    float drag_distance = 4.0f;
    int drag_cost = 1;
};
struct Dragging {
    static constexpr u32 magic_number = 0xd2a90000;
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

struct EmitterComponent {
    EmitterGPU* emitter;
    Timer* timer;
};

void inspect_components(Scene* scene, entt::entity entity);
void preview_3d_components(Scene* scene, entt::entity entity);

}
