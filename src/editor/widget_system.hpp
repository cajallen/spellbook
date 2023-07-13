#pragma once

#include <GLFW/glfw3.h>

#include "general/umap.hpp"
#include "general/math/geometry.hpp"
#include "general/math/matrix.hpp"

namespace spellbook {

namespace WidgetSystem {

extern umap<uint64, float> depths;
extern uint64 pressed_id;

void setup();
void update();

struct CircleProjectInfo {
    float distance = 0.0f;
    float angle = 0.0f;
    float visual_angle = 0.0f;
    v3 position = v3(0.0f);
};

struct LineProjectInfo {
    float distance = 0.0f;
    float axis_value = 0.0f;
    float visual_axis_value = 0.0f;
    v3 position = v3(0.0f);
};

struct DotProjectInfo {
    float distance = 0.0f;
};

struct PlaneProjectInfo {
    v3 position = v3(0.0f);
};

struct Mouse3DInfo {
    m44 model;
    m44 mvp;
    v2 uv_position;
    ray3 os_ray;
    ray3 os_center_ray;

    v2 viewport_size;
};

CircleProjectInfo mouse_to_3d_circle(const Mouse3DInfo& mouse, float radius, int axis, range angle_range);
LineProjectInfo mouse_to_3d_line(const Mouse3DInfo& mouse, float radius, int axis);
PlaneProjectInfo mouse_to_3d_plane(const Mouse3DInfo& mouse, int axis);
DotProjectInfo mouse_to_3d_dot(const Mouse3DInfo& mouse);

}

}
