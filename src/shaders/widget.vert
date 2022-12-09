#version 460
#pragma shader_stage(vertex)

#include "include.glsli"

layout (location = 0) in vec3 vin_position;
layout (location = 1) in vec3 vin_color;

layout (binding = CAMERA_BINDING) uniform CameraData {
    mat4 view;
    mat4 projection;
    vec4 camera_position;
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
    vec3 color;
} vout;


void main() {
    vout.color = vin_color;
    gl_Position = projection * view * vec4(vin_position, 1.0);
}
