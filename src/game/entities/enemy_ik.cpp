#include "enemy_ik.hpp"

#include <imgui.h>

#include "extension/fmt.hpp"
#include "general/matrix_math.hpp"
#include "game/scene.hpp"
#include "game/systems.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

SpiderControllerSettings::SpiderControllerSettings() : bone_names{"foot.pn", "foot.pp", "foot.np", "foot.nn"} {
    desired_dist_from_center = 0.22f;
    step_time = 0.25f;
    
    default_control_points[0] = math::normalize(v3(1.0f, -1.0f, 0.f));
    default_control_points[1] = math::normalize(v3(1.0f, 1.0f, 0.f));
    default_control_points[2] = math::normalize(v3(-1.0f, 1.0f, 0.f));
    default_control_points[3] = math::normalize(v3(-1.0f, -1.0f, 0.f));
}

SpiderController::SpiderController(SpiderControllerSettings& settings) : settings(settings) {
    initialized = false;
    
    quad_norm = v3::Z;
    extra_z = 0.0f;
    
    old_pos = {};
    old_stuff_time = 0.0f;
    velocity = {};

    last_leg_moved = 0;

    updated_desired_dist_from_center = settings.desired_dist_from_center;
}

void SpiderController::initialize_if_needed(const m44& tfm) {
    if (!initialized) {
        for (int i = 0; i < 4; i++) {
            world_starts[i] = math::apply_transform(tfm, updated_desired_dist_from_center * settings.default_control_points[i]);
            world_targets[i] = math::apply_transform(tfm, updated_desired_dist_from_center * settings.default_control_points[i]);
            lerp_t[i] = 1.0f;
        }
        initialized = true;
    }
}

void SpiderController::update_velocity(Scene* scene, const LogicTransform& logic_tfm) {
    if (math::abs(scene->time - old_stuff_time) > 0.01f && old_stuff_time > 0.0f)
        velocity = (logic_tfm.position - old_pos) / (scene->time - old_stuff_time);
    speed = math::length(velocity);
    old_pos = logic_tfm.position;
    old_stuff_time = scene->time;

    settings.step_ahead_dist = speed > 0.1f ? 0.5f * speed * settings.step_time : 0.15f;
    updated_desired_dist_from_center = updated_desired_dist_from_center < settings.desired_dist_from_center ?
        math::min(updated_desired_dist_from_center + scene->delta_time * 0.5f, settings.desired_dist_from_center) :
        math::max(updated_desired_dist_from_center - scene->delta_time * 0.5f, settings.desired_dist_from_center);
}


void SpiderController::update_desire(const m44& tfm) {
    for (int i = 0; i < 4; i++) {
        world_desired[i] = math::apply_transform(tfm, updated_desired_dist_from_center * settings.default_control_points[i]);
                
        if (lerp_t[i] < 1.0f) {
            want_move_leg[i] = false;
            continue;
        }
                
        float dist_to_desired = math::distance(get_control_point(i), world_desired[i]);
        if (dist_to_desired > 0.05f) {
            want_move_leg[i] = true;
        }
        else if (dist_to_desired > 0.02f && math::length(velocity) <= 0.01f) {
            want_move_leg[i] = true;
        }
    }

    want_move_set[0] = want_move_leg[0] || want_move_leg[2];
    want_move_set[1] = want_move_leg[1] || want_move_leg[3];
    if (is_set_moving(0))
        want_move_set[1] = false;
    if (is_set_moving(1))
        want_move_set[0] = false;
    if (want_move_set[0] && want_move_set[1]) {
        if (last_leg_moved == 0)
            want_move_set[0] = false;
        else
            want_move_set[1] = false;
    }
}

void SpiderController::resolve_desire() {
    for (int i = 0; i < 4; i++) {
        if (want_move_set[i%2]) {
            world_starts[i] = world_targets[i];
            lerp_t[i] = 0.0f;
            last_leg_moved = i % 2;
        }
    }
}

void SpiderController::update_target(Scene* scene, const LogicTransform& logic_tfm) {
    for (int i = 0; i < 4; i++) {
        if (is_moving(i)) {
            float step_up = calculate_step_up(scene, entt::null, v3(world_targets[i].xy - v2(0.5f, 0.5f), logic_tfm.position.z));
            world_targets[i] = world_desired[i] + settings.step_ahead_dist * (math::length(velocity) > 0.01f ? math::normalize(velocity) : v3(0.0f));
            world_targets[i].z = get_foot_height(scene, world_targets[i].xy, logic_tfm.position.z + 0.5f) + step_up;
        }
    }
}

bool SpiderController::check_all_targets_set() {
    bool all_set = true;
    for (int i = 0; i < 4; i++)
        if (world_targets[i] == v3(0.0f))
            all_set = false;
    return all_set;
}

void SpiderController::update_constraints(Scene* scene, const Model& model, const m44& tfm_inv) {
    for (int i = 0; i < 4; i++) {
        Bone* bone = model.model_cpu->skeleton->find_bone(settings.bone_names[i]);
        if (!bone)
            continue;

        v3 local_current_target = math::apply_transform(tfm_inv, get_control_point(i));
        IKTarget ik_target = {bone, local_current_target, 3};
        apply_constraints(ik_target);
        lerp_t[i] += scene->delta_time / settings.step_time;
    }
}


void SpiderController::update_transform_linkers() {
    quad_norm = v3::Z;
    extra_z = 0.0f;

    // v3 avg = 0.25f * (world_targets[0] + world_targets[1] + world_targets[2] + world_targets[3]);
    v3 tri_norm1 = math::ncross(world_targets[1] - world_targets[0], world_targets[2] - world_targets[0]);
    v3 tri_norm2 = math::ncross(world_targets[2] - world_targets[0], world_targets[3] - world_targets[0]);
    quad_norm = math::normalize(tri_norm1 + tri_norm2);
    if (math::is_nan(quad_norm))
        quad_norm = v3::Z;
    extra_z = 1.0f - math::dot(quad_norm, v3::Z);

    assert_else(!math::is_nan(extra_z));

    settings.desired_dist_from_center = math::dot(quad_norm, v3::Z) > 0.99f ? 0.26f : 0.31f;
}


float get_foot_height(Scene* scene, v2 foot_pos, float init_height) {
    const umap<v3i, Direction>& ramps = scene->map_data.ramps;
    v3 out_pos;
    v3i out_cube;
    bool found = ray_intersection(scene->map_data.solids, ray3{v3(foot_pos, init_height), v3(0.0f, 0.0f, -1.0f)}, out_pos, out_cube,
        [ramps](ray3 r, v3i v, v3& out_pos) {
            if (!ramps.contains(v))
                return false;

            switch (ramps.at(v)) {
                case Direction_PosX: {
                    float z_offset = math::fract(r.origin.x);
                    out_pos = v3(r.origin.xy, float(v.z-1) + z_offset);
                } break;
                case Direction_NegX: {
                    float z_offset = 1.0f - math::fract(r.origin.x);
                    out_pos = v3(r.origin.xy, float(v.z-1) + z_offset);
                } break;
                case Direction_PosY: {
                    float z_offset = math::fract(r.origin.y);
                    out_pos = v3(r.origin.xy, float(v.z-1) + z_offset);
                } break;
                case Direction_NegY: {
                    float z_offset = 1.0f - math::fract(r.origin.y);
                    out_pos = v3(r.origin.xy, float(v.z-1) + z_offset);
                } break;
            }
            
            return true;
        });

    if (found)
        return out_pos.z;
    else
        return init_height;
}

void SpiderController::inspect() {
    ImGui::Checkbox("Initialized", &initialized);
    ImGui::DragFloat3("Quad Norm", quad_norm.data, 0.01f);
    ImGui::DragFloat("Extra Z", &extra_z, 0.01f);
    ImGui::DragFloat3("Old Pos", old_pos.data, 0.01f);
    ImGui::DragFloat("Old Time", &old_stuff_time, 0.01f);
    for (int i = 0; i < 4; i++) {
        ImGui::DragFloat3(fmt_("World Start {}", i).c_str(), world_starts[i].data, 0.01f);
        ImGui::DragFloat3(fmt_("World Target {}", i).c_str(), world_targets[i].data, 0.01f);
        ImGui::DragFloat(fmt_("t {}", i).c_str(), &lerp_t[i], 0.01f);
    }
    ImGui::Text("Last Leg: %s", last_leg_moved ? "left" : "right");

}

}
