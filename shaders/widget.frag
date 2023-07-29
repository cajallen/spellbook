#version 450
#pragma shader_stage(fragment)

#include "include.glsli"

layout (location = 0) in VS_OUT {
    vec3 color;
} fin;

layout (location = 0) out vec4 fout_color;

void main() {
    fout_color = vec4(fin.color, 1.0);
}