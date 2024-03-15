#version 450
#pragma shader_stage(fragment)

#include "include.glsli"

layout (location = 0) in VS_OUT {
    vec2 position;
    vec4 color;
    vec2 uv;
} fin;

layout (location = 0) out vec4 fout_color;

layout(binding = TEXTURE_BINDING) uniform usampler2D s_texture;

void main() {
    float s = texture(s_texture, fin.uv).r / 255.0;
    fout_color = vec4(fin.color.rgb, s * fin.color.a);
}