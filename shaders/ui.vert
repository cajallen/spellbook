#version 460
#pragma shader_stage(vertex)

#include "include.glsli"

layout (location = 0) in vec3 vin_position;
layout (location = 1) in vec2 vin_uv;
layout (location = 2) in uvec4 vin_color;

layout (binding = VIEW_BINDING) uniform ViewData {
    mat4 screen_pixels_to_NDC;
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
    vec2 position;
    vec4 color;
    vec2 uv;
} vout;

void main() {
    vec4 h_position = screen_pixels_to_NDC * vec4(vin_position, 1.0);
    vout.position = vin_position.xy;
    vout.color = vin_color / 255.0;
    vout.uv = vin_uv;
    gl_Position = h_position;
}
