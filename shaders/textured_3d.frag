#version 450
#pragma shader_stage(fragment)

#include "include.glsli"

layout (location = 0) in VS_OUT {
    vec3 position;
    vec3 color;
    vec2 uv;
    mat3 TBN;
    flat uint id;
} fin;

layout (location = 0) out vec4 fout_color;
layout (location = 1) out vec4 fout_emissive;
layout (location = 2) out vec4 fout_normal;
layout (location = 3) out uvec4 fout_id;

layout(binding = MATERIAL_BINDING) uniform MaterialData {
	vec4 base_color_tint; 
	vec4 emissive_tint;
    vec4 roughness_metallic_normals_scale;
};
layout(binding = BASE_COLOR_BINDING) uniform sampler2D s_base_color; 
layout(binding = ORM_BINDING) uniform sampler2D s_metallic_roughness; 
layout(binding = NORMAL_BINDING) uniform sampler2D s_normal; 
layout(binding = EMISSIVE_BINDING) uniform sampler2D s_emissive;

vec2 calculate_uv() {
    return fin.uv * roughness_metallic_normals_scale.w;
}

void main() {
    vec2 uv = calculate_uv();

    fout_color = texture(s_base_color, uv) * base_color_tint;
    vec3 normal_input = texture(s_normal, uv).rgb * 2.0 - 1.0;
    normal_input.b /= max(roughness_metallic_normals_scale.z, 0.00001);
    fout_normal = vec4(normalize(fin.TBN * normal_input), texture(s_metallic_roughness, uv).g * roughness_metallic_normals_scale.r);

    vec4 emissive_input = texture(s_emissive, uv);
    fout_emissive = vec4(emissive_input.rgb * emissive_input.a * emissive_tint.rgb * emissive_tint.a + fin.color, 1.0);
    fout_id = uvec4(fin.id, fin.id, fin.id, fin.id);
}