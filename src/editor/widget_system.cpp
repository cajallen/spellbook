#include "widget_system.hpp"

#include "game/input.hpp"
#include "general/math.hpp"
#include "general/matrix_math.hpp"

namespace spellbook {

bool widget_click(ClickCallbackArgs args) {
    if (args.action != GLFW_RELEASE) {
        float closest_depth = FLT_MAX;
        u64 closest_id;
        for (auto& [id, depth] : WidgetSystem::depths) {
            if (depth < closest_depth) {
                closest_depth = depth;
                closest_id = id;
            }
        }
    
        if (closest_depth < FLT_MAX) {
            WidgetSystem::pressed_id = closest_id;
            return true;
        }
    }
    else if (args.action == GLFW_RELEASE) {
        WidgetSystem::pressed_id = 0;
    }
    return false;
}


namespace WidgetSystem {

umap<u64, float> depths;
u64 pressed_id;

void setup() {
    Input::add_callback(InputCallbackInfo{widget_click, 40, "widget_system", nullptr});
}

void update() {
    for (auto& [id, depth] : WidgetSystem::depths) {
        depth = FLT_MAX;
    }
}

CircleProjectInfo mouse_to_3d_circle(const Mouse3DInfo& mouse, float radius, int axis, range angle_range) {
    CircleProjectInfo return_info;

    // We intersect our OS ray with the axis' plane, because the intersect is in OS, we can just atan it for angle
    if (math::abs(mouse.os_ray.dir[axis]) < 0.001f) {
        return_info.angle = 0.0f;
    } else {
        float t = -mouse.os_ray.origin[axis] / mouse.os_ray.dir[axis];
        v3 os_intersect = mouse.os_ray.origin + t * mouse.os_ray.dir;
        return_info.angle = math::atan2(os_intersect[(axis + 2) % 3], os_intersect[(axis + 1) % 3]);
    }
    return_info.visual_angle = math::clamp_angle(return_info.angle, angle_range);

    v3 os_projected(0.0f);
    os_projected[(1 + axis) % 3] = radius * math::cos(return_info.visual_angle);
    os_projected[(2 + axis) % 3] = radius * math::sin(return_info.visual_angle);

    v4 h_projected = mouse.model * v4(os_projected, 1.0);
    return_info.position = h_projected.xyz / h_projected.w;
    v4 h_screen_position = mouse.mvp * v4(os_projected, 1.0);
    v2 uv_screen_position = math::to_unsigned_range(h_screen_position.xy / h_screen_position.w);
    return_info.distance = math::length((uv_screen_position - mouse.uv_position) * mouse.viewport_size);

    return return_info;
}

LineProjectInfo mouse_to_3d_line(const Mouse3DInfo& mouse, float radius, int axis) {
    LineProjectInfo return_info;

    auto axis_ray = ray3(v3(0.0f), v3(0.0f));
    axis_ray.dir[axis] = 1.0f;
    
    return_info.axis_value = math::line_intersection_3d(axis_ray, mouse.os_ray);
    return_info.visual_axis_value = math::clamp(return_info.axis_value, math::min(0.0f, radius), math::max(0.0f, radius));
    
    v3 axis_projected = v3(0.0f);
    axis_projected[axis] = return_info.visual_axis_value;
    v4 h_screen_axis = mouse.mvp * v4(axis_projected, 1.0f);
    v2 uv_axis_position = math::to_unsigned_range(h_screen_axis.xy / h_screen_axis.w);
    return_info.distance = math::length((uv_axis_position - mouse.uv_position) * mouse.viewport_size);
    return_info.position[axis] = return_info.visual_axis_value;
    
    return return_info;
}

DotProjectInfo mouse_to_3d_dot(const Mouse3DInfo& mouse) {
    DotProjectInfo return_info;

    v3 projected = v3(0.0f);
    v4 h_screen_axis = mouse.mvp * v4(projected, 1.0f);
    v2 uv_axis_position = math::to_unsigned_range(h_screen_axis.xy / h_screen_axis.w);
    return_info.distance = math::length((uv_axis_position - mouse.uv_position) * mouse.viewport_size);
    
    return return_info;
}

PlaneProjectInfo mouse_to_3d_plane(const Mouse3DInfo& mouse, int axis) {
    PlaneProjectInfo return_info;

    v3 os_intersect = math::intersect_axis_plane(mouse.os_ray, axis, 0.0f);
    
    v4 h_projected = mouse.model * v4(os_intersect, 1.0);
    return_info.position = h_projected.xyz / h_projected.w;
    
    return return_info;
}

}

}
