#version 450
#pragma shader_stage(vertex)

#include "include.glsli"

layout (location = 0) in vec3 vin_position;
layout (location = 1) in vec3 vin_normal;
layout (location = 2) in vec3 vin_tangent;
layout (location = 3) in vec3 vin_color;
layout (location = 4) in vec2 vin_uv;

layout (binding = PARTICLES_BINDING) buffer Particles {
    uint head;
    Particle particles[];
};

layout (binding = CAMERA_BINDING) uniform CameraData {
    mat4 vp;
};

layout(binding = SPARE_BINDING_1) uniform sampler2D color_table;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
    vec3 position;
    vec3 color;
    vec2 uv;
    mat3 TBN;
    flat uint id;
} vout;

void main() {
    uint index = gl_InstanceIndex;
    Particle particle = particles[index];

    float scale = particle.position_scale.w * max(smoothstep(0.0, particle.falloff, particle.life / particle.life_total), 0.001);
    mat4 M;
    M[0] = vec4(scale, 0.0, 0.0, 0.0);
    M[1] = vec4(0.0, scale, 0.0, 0.0);
    M[2] = vec4(0.0, 0.0, scale, 0.0);
    M[3] = vec4(particle.position_scale.x, particle.position_scale.y, particle.position_scale.z, 1.0);
    if (particle.life < 0)
        M[3][3] = 1.0 / 0.0;

    if (particles[index].alignment.w > 0.0) {
        vec3 a = particles[index].alignment.xyz;
        vec3 b = normalize(particles[index].velocity_damping.xyz);
        vec3 vr = b - dot(a, b) * a;
        vec3 basis_2 = normalize(vr);
        vec3 basis_3 = normalize(cross(b, a));
        vec3 basis_1 = normalize(cross(basis_2, basis_3));
        mat3 rotation = mat3(basis_1, basis_2, basis_3);
        M = M * mat4(rotation);
    }
    vec4 h_position = M * vec4(vin_position, 1.0);
    vout.position = h_position.xyz / h_position.w;
    
    mat3 N = inverse(transpose(mat3(M)));
    vec3 n = normalize(N * vin_normal);
    vec3 t = normalize(N * vin_tangent);
    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    vout.TBN      = mat3(t, b, n);

    vout.uv = vin_uv;
    vout.color = texture(color_table, vec2(particle.color_x, 1.0 - particle.life / particle.life_total)).rgb;
    gl_Position = vp * h_position;
}