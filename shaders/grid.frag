#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec4 fin_position;
layout (location = 0) out vec4 fout_color;
layout (location = 1) out vec4 fout_emissive;
layout (location = 2) out vec4 fout_normal;
layout (location = 3) out uvec4 fout_id;

layout(binding = 1) uniform sampler2D s_grid; 

void main() {
    vec2 uv = fin_position.xy;
    
    float value = 1.0 - texture(s_grid, uv).r;
    vec3 line_value = vec3(0.0);
    float alpha_factor = 1.0;

    float mipmapLevel = textureQueryLod(s_grid, uv).x;
    float res = 1024.0 / pow(2.0, mipmapLevel);

    float color_width = 1.5 / res;
    if (-color_width < uv.y && uv.y < color_width) { // y = 0, we're the x axis
        if (uv.x < 0) {
            if (fract(uv.x * 8.0) < 0.5) {
                line_value = vec3(1,0,0);
                alpha_factor = 1.0;
            }
        } else {
            line_value = vec3(1,0,0);
            alpha_factor = 1.0;
        }
    }
    if (-color_width < uv.x && uv.x < color_width) { // x = 0, we're the y axis
        if (uv.y < 0) {
            if (fract(uv.y * 8.0) < 0.5) {
                line_value = vec3(0,1,0);
                alpha_factor = 1.0;
            }
        } else {
            line_value = vec3(0,1,0);
            alpha_factor = 1.0;
        }
    }
    
    if (alpha_factor * value < 0.01)
        discard;

    fout_color = vec4(line_value, alpha_factor * value);
    fout_emissive = vec4(line_value, alpha_factor * value);
    fout_normal = vec4(0.0,0.0,1.0,1.0);
    fout_id = uvec4(-1,-1,-1,-1);
}