#pragma once

#include <entt/entity/entity.hpp>
#include <entt/signal/sigh.hpp>

#include "general/string.hpp"
#include "general/umap.hpp"
#include "general/math/geometry.hpp"
#include "general/math/quaternion.hpp"
#include "general/math/matrix.hpp"

#include "renderer/assets/model.hpp"

namespace spellbook {

struct Scene;
struct EmitterCPU;
struct EmitterGPU;
struct Timer;
struct Stat;
struct StatEffect;

struct Name {
    string name;
};

struct Model {
    std::unique_ptr<ModelCPU> model_cpu = nullptr;
    ModelGPU model_gpu = {};
};

struct StaticModel {
    vector<StaticRenderable*> renderables;
};

struct LogicTransform {
    v3 position = v3(0.0f);
    v3 normal = v3::Z;
    float yaw;
};

struct Grounded {
    float step_up;
};

struct ModelTransform {
    v3    translation = v3(0.0f);
    quat  rotation    = quat();
    v3    scale       = v3(1.0f);
    m44   transform;
    bool  transform_dirty = true;
    bool  renderable_dirty = true;

    void set_translation(const v3& v);
    void set_rotation(const quat& q);
    void set_scale(const v3& v);
    void set_dirty();
    const m44& get_transform();
};

struct TransformLink {
    v3 offset = v3(0.5f);
    float rotate_speed = 2.0f;
};

struct GridSlot {
    bool path = false;
    bool ramp = false;
    bool standable = false;
    Direction direction = Direction_Up;
    uset<entt::entity> linked;
};

struct FloorOccupier {
};

struct Health {
    Scene* scene;
    entt::entity entity;
    float value = 0.0f;
    float buffer_value = 0.0f;
    
    FilePath emitter_cpu_path;

    std::unique_ptr<Stat> max_health;
    std::unique_ptr<Stat> damage_taken_multiplier;
    std::unique_ptr<Stat> regen;
    umap<entt::entity, std::unique_ptr<Stat>> dots;

    entt::sigh<void(Scene*, entt::entity, entt::entity, float)> damage_signal;
    
    Health(float health_value, Scene* scene, entt::entity e, const FilePath& hurt_emitter_path = {});
    void damage(entt::entity damager, float amount, v3 direction);
    void apply_dot(entt::entity damager, uint64 id, const StatEffect& effect, EmitterCPU* emitter = nullptr);
};

struct Killed {
    bool drop = false;
    float when = -FLT_MAX;
};

struct Draggable {
    float drag_height;
};
struct Dragging {
    static constexpr uint32 magic_number = 0xd2a90000;
    float drag_height;
    float start_time = 0.0f;
    v3  start_logic_position = v3(0.0f);
    v3  start_intersect = v3(0.0f);
    
    v3 target_position = v3(0.0f);
    v3 potential_logic_position = v3(0.0f);

    static uint64 get_drag_tag_id(entt::entity entity) {
        return uint64(Dragging::magic_number) << 32ull | uint64(entity);
    }
};

struct ForceDragging {
    v3 target;
    float time;
};

struct Collision {
    float                radius = 0.0f;
    uset<entt::entity> with = {};
};

struct EmitterComponent {
    Scene* scene;
    umap<uint64, EmitterGPU*> emitters;

    void add_emitter(uint64 id, const EmitterCPU& emitter_cpu);
    void remove_emitter(uint64 id);
};

struct CastingPlatform {
    bool charged = true;
};

struct AddToInspect {};

void inspect_components(Scene* scene, entt::entity entity);
void preview_3d_components(Scene* scene, entt::entity entity);

void remove_dragging_impair(Scene* scene, entt::entity entity);
void remove_dragging_impair(entt::registry& reg, entt::entity entity);

entt::entity setup_basic_unit(Scene* scene, const FilePath& model_path, v3 location, float health_value, const FilePath& hurt_path);

void on_gridslot_create(Scene& scene, entt::registry& registry, entt::entity entity);
void on_gridslot_destroy(Scene& scene, entt::registry& registry, entt::entity entity);
void on_dragging_create(Scene& scene, entt::registry& registry, entt::entity entity);
void on_dragging_destroy(Scene& scene, entt::registry& registry, entt::entity entity);
void on_model_destroy(Scene& scene, entt::registry& registry, entt::entity entity);
void on_emitter_component_destroy(Scene& scene, entt::registry& registry, entt::entity entity);

void on_forcedrag_create(Scene& scene, entt::registry& registry, entt::entity entity);
void on_forcedrag_destroy(Scene& scene, entt::registry& registry, entt::entity entity);


}
