#version 460
#pragma shader_stage(vertex)

layout (location = 0) in vec3 vin_position;
layout (location = 1) in vec3 vin_normal;
layout (location = 2) in vec3 vin_tangent;
layout (location = 3) in vec3 vin_color;
layout (location = 4) in vec2 vin_uv;

layout (binding = 0) uniform CameraData {
	mat4 view;
	mat4 projection;
	vec4 camera_position;
};

layout (binding = 2) buffer readonly Model {
	mat4 model[];
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
    vec3 position;
    vec3 color;
    vec2 uv;
    mat3 TBN;
} vout;


void main() {
    vec4 h_position = model[gl_BaseInstance] * vec4(vin_position, 1.0);
    vout.position = h_position.xyz;
    
    mat3 N = inverse(transpose(mat3(model[gl_BaseInstance])));
	vec3 n = normalize(N * vin_normal);
	vec3 t = normalize(N * vin_tangent);
	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);
	vout.TBN      = mat3(t, b, n);
    
	vout.uv = vin_uv;
	vout.color = vin_color;
    gl_Position = projection * view * h_position;
}
