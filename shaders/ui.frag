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

layout(push_constant) uniform uPushConstant {
    bool single_channel;
} pc;

void main() {
    vec4 tex = texture(s_texture, fin.uv);
    tex.rgb = linear_to_srgb(tex.rgb);

    if (pc.single_channel) {
        fout_color = vec4(fin.color.rgb, tex.r * fin.color.a);
    } else {
        fout_color = fin.color * tex;
    }
}