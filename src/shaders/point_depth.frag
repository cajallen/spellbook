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

//layout (location = 0) out vec4 fout_color;

void main() {
    //float light_distance = length(fin.position - camera_position.xyz);
    //gl_FragDepth = (light_distance - clip_planes[0]) / (clip_planes[1] - clip_planes[0]);
    
//    vec3 light_to_fragment = normalize(fin.position - camera_position.xyz);
//    float NdotL = dot(-light_to_fragment, normalize(fin.TBN * vec3(0,0,1)));
//    fout_color = vec4(vec3(NdotL) , 1.0);
}