#version 460
#pragma shader_stage(vertex)

#include "include.glsli"

layout (location = 0) in vec2 vin_position;
layout (location = 1) in vec2 vin_uv;
layout (location = 2) in uvec4 vin_color;

layout (binding = CAMERA_BINDING) uniform CameraData {
	mat4 vp;
};

layout (binding = MODEL_BINDING) buffer readonly Model {
	mat4 model[];
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
    vec3 position;
    vec4 color;
    vec2 uv;
} vout;


void main() {
    vec4 h_position = model[gl_InstanceIndex] * vec4(vin_position, 0.0, 1.0);
	vout.position = h_position.xyz / h_position.w;
    
	vout.uv = vin_uv;
	vout.color = vin_color / 255.0;
    gl_Position = vp * h_position;
}
