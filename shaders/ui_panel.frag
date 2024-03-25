#version 450
#pragma shader_stage(fragment)

#include "include.glsli"

layout (location = 0) in VS_OUT {
    vec2 position;
    vec4 color;
    vec2 uv;
} fin;

layout (location = 0) out vec4 fout_color;

layout(binding = TEXTURE_BINDING) uniform sampler2D s_texture;

layout(binding = MATERIAL_BINDING) uniform MaterialData {
    vec4 color1;
    vec4 color2;
    vec3 vignette_amount;
};

layout(push_constant) uniform uPushConstant {
    bool single_channel;
} pc;

void main() {
    vec4 tex = texture(s_texture, fin.uv);

    vec4 color = mix(color1, color2, tex.r);
    float vignette_amount = vignette_amount.z * smoothstep(vignette_amount.x, vignette_amount.y, fin.color.r);
    color.rgb *= (1.0f - vignette_amount);

    fout_color = color;
}