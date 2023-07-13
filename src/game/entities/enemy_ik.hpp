#pragma once

#include <entt/entity/fwd.hpp>

#include "general/math/geometry.hpp"
#include "general/math/matrix.hpp"
#include "general/math/math.hpp"

namespace spellbook {

struct Scene;
struct LogicTransform;
struct ModelTransform;
struct Model;

struct SpiderControllerSettings {
    string bone_names[4];

    float desired_dist_from_center;
    float step_ahead_dist;
    float step_time;
    
    v3 default_control_points[4];

    SpiderControllerSettings();
};

struct SpiderController {
    SpiderControllerSettings& settings;
    
    bool initialized;
    
    v3 quad_norm;
    float extra_z;
    
    v3 old_pos;
    float old_stuff_time;
    v3 velocity;
    float speed;
    
    v3 world_starts[4];
    v3 world_targets[4];
    float lerp_t[4];
    uint8 last_leg_moved;

    v3 world_desired[4];
    bool want_move_leg[4];
    bool want_move_set[2];

    float updated_desired_dist_from_center;

    SpiderController(SpiderControllerSettings& settings);
    
    v3 get_control_point(int i) {
        constexpr float height = 0.1f;
        float t = math::clamp(lerp_t[i], {0.0f, 1.0f});
        return math::lerp(t, {world_starts[i], world_targets[i]})
            + v3(0.0f, 0.0f,
                height *
                math::distance(world_starts[i], world_targets[i]) *
                2.0f * math::sqrt(0.25f - math::pow(t - 0.5f, 2.0f)));
    }

    bool is_moving(uint32 i) const {
        return lerp_t[i] >= 0.0f && lerp_t[i] < 1.0f;
    }
    bool is_set_moving(uint32 i) const {
        return is_moving(i) || is_moving(i + 2);
    }

    void initialize_if_needed(const m44& tfm);
    void update_velocity(Scene* scene, const LogicTransform& logic_tfm);
    void update_desire(const m44& tfm);
    void resolve_desire();
    void update_target(Scene* scene, const LogicTransform& logic_tfm);
    bool check_all_targets_set();
    void update_transform_linkers();
    void update_constraints(Scene* scene, const Model& model, const m44& tfm_inv);

    
    void inspect();
};

float get_foot_height(Scene* scene, v2 foot_pos, float init_height);

}