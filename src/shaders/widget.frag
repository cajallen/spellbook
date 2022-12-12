#version 450
#pragma shader_stage(fragment)

#include "include.glsli"

layout (location = 0) in VS_OUT {
    vec3 color;
} fin;

layout (location = 0) out vec4 fout_color;

layout (binding = CAMERA_BINDING) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 camera_position;
};
layout(binding = SCENE_BINDING) uniform SceneData {
    vec4 ambient;
    vec4 fog;
    vec4 sun_direction_intensity;
    vec4 rim_alpha_width_start;
};

void main() {
    fout_color = vec4(fin.color, 1.0);
}