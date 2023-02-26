#pragma once

#include "general/geometry.hpp"
#include "general/quaternion.hpp"
#include "general/matrix.hpp"

namespace spellbook {

struct RenderScene;

enum Operation {
    Operation_TranslateX      = 0,
    Operation_TranslateY      = 1,
    Operation_TranslateZ      = 2,
    Operation_TranslateXY     = 3,
    Operation_TranslateYZ     = 4,
    Operation_TranslateZX     = 5,
    Operation_TranslateCamera = 6,
    Operation_RotateX         = 7,
    Operation_RotateY         = 8,
    Operation_RotateZ         = 9,
    Operation_RotateCamera    = 10,
    Operation_Count
};

struct PoseWidgetState {
    m44 model;
    
    u32 enabled = ~0u;
    u32 hovered =  0u;
    u32 pressed =  0u;
    u32 hidden  =  0u;
    array<float, Operation_Count> depth;
    
    array<float, Operation_Count> value;
    array<float, Operation_Count> clicked_value;

    v3 clicked_position;
    quat clicked_rotation;
};

struct PoseWidgetSettings {
    RenderScene& render_scene;
    
    float transform1d_size = 0.5f;
    float transform2d_start = 0.05f;
    float transform2d_size = 0.1f;
    float rotation_size = 0.4f;

    float line_radius = 0.01f;
    float selection_radius = 10.0f;

    float cutoff_warning = 0.04f;
    float rotation_dot_cutoff = 0.15f;
    float rotation_dot_aligned = 0.97f;
    float transform2d_dot_cutoff = 0.20f;
    range rotation_flatstep_dot = {0.85f, 0.95f};
    range rotation_flatstep_percentage = {0.55f, 1.0f};

    float color_neutral = 0.1f;
    float color_neutral_hovered = 0.5f;
    float color_axis = 1.0f;

    u32 disabled = 0;
};

bool pose_widget(u64 id, v3* location, quat* rotation, const PoseWidgetSettings& settings, m44* model = nullptr, PoseWidgetState* widget_state = nullptr);
void _pose_widget(PoseWidgetState* widget_state, const PoseWidgetSettings& settings);
void inspect(PoseWidgetState* state);

}
