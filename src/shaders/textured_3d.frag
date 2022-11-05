#version 450
#pragma shader_stage(fragment)


layout (location = 0) in VS_OUT {
    vec3 position;
    vec3 color;
    vec2 uv;
    mat3 TBN;
} fin;

layout (location = 0) out vec4 fout_color;
layout (location = 1) out vec4 fout_normal;
layout (location = 2) out uvec4 fout_id;

layout (binding = 0) uniform CameraData {
	mat4 view;
	mat4 projection;
	vec4 camera_position;
};
layout(binding = 1) uniform SceneData {
	vec4 ambient;
    vec4 fog;
    vec4 sun_direction_intensity; 
    vec2 rim_amount_start;
};
layout(binding = 3) uniform MaterialData {
	vec4 base_color_tint; 
	vec4 emissive_tint; 
	vec4 roughness_metallic_normals_scale;
};
layout(binding = 4) uniform sampler2D s_base_color; 
layout(binding = 5) uniform sampler2D s_metallic_roughness; 
layout(binding = 6) uniform sampler2D s_normal; 
layout(binding = 7) uniform sampler2D s_emissive; 

layout(push_constant) uniform uPushConstant {
    int selection_id;
} pc;

vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

float map(float value, float min_in, float max_in, float min_out, float max_out) {
    return min_out + (value - min_in) * (max_out - min_out) / (max_in - min_in);
}

float map_contained(float value, float min_in, float max_in, float min_out, float max_out) {
    if (value < min_in || value >= max_in)
        return 0.0;
    return map(value, min_in, max_in, min_out, max_out);
}


vec2 calculate_uv() {
    return fin.uv * roughness_metallic_normals_scale.w;
}

vec3 calculate_lighting() {
    vec2 uv = calculate_uv();

	vec3 normal_input = texture(s_normal, uv).rgb * 2.0 - 1.0;
	normal_input.b /= max(roughness_metallic_normals_scale.z, 0.00001);
    normal_input = normalize(normal_input);
	vec3 normal = normalize(fin.TBN * normal_input);
    fout_normal = vec4(normal, 1.0);

    vec3 to_light = normalize(sun_direction_intensity.xyz);
    vec3 view_dir = normalize(fin.position - camera_position.xyz);
    float roughness = texture(s_metallic_roughness, uv).g * roughness_metallic_normals_scale.x;
    float smoothness = 1.0 - roughness;
    smoothness = clamp(smoothness, 0.001, 1.00);

    vec4 albedo = texture(s_base_color, uv) * base_color_tint;
    
    float NdotL = dot(normal, to_light);

    const float transition = 0.05;
    const float transition1_start = -0.2;
    const float transition2_start = -transition1_start - transition;
    const float range = 0.15;
    const float mid_out = 0.5;
    
    float toon_diffuse = 0.0;
    if (NdotL < transition1_start) {
        float out_min = 0.0;
        float out_max = range;
        toon_diffuse = map(NdotL, -1.0, transition1_start, out_min, out_max);
    }
    else if (NdotL < transition1_start + transition) {
        // transition1
        float out_min = range;
        float out_max = mid_out - 0.5 * range;
        toon_diffuse = map(NdotL, transition1_start, transition1_start + transition, out_min, out_max);
    }
    else if (NdotL < transition2_start) {
        // middle
        float out_min = mid_out - 0.5 * range;
        float out_max = mid_out + 0.5 * range;
        toon_diffuse = map(NdotL, transition1_start + transition, transition2_start, out_min, out_max);
    }
    else if (NdotL < transition2_start + transition) {
        // transition2
        float out_min = mid_out + 0.5 * range;
        float out_max = 1.0 - range;
        toon_diffuse = map(NdotL, transition2_start, transition2_start + transition, out_min, out_max);
    }
    else {
        // high
        float out_min = 1.0 - range;
        float out_max = 1.0;
        toon_diffuse = map(NdotL, transition2_start + transition, 1.0, out_min, out_max);
    }
    
    vec3 reflection_dir = reflect(to_light, normal);
    float specular = pow(clamp(dot(view_dir, reflection_dir), 0.0, 1.0), smoothness * 100);
    float toon_specular = smoothstep(0.4, 0.5, specular);

    float rim_dot = 1.0 - dot(-view_dir, normal);
    float rim_intensity = rim_dot * pow(NdotL, rim_amount_start.y);
    //float rim_intensity = smoothstep(rim_amount_start.y - 0.01, rim_amount_start.y + 0.01, rim_dot * NdotL);
    rim_intensity = smoothstep(rim_amount_start.y - 0.01, rim_amount_start.y + 0.01, rim_intensity);

    vec3 diffuse_real = albedo.rgb * clamp(toon_diffuse, 0.0, 1.0) + albedo.rgb * ambient.rgb * ambient.a;
    vec3 specular_real = vec3(toon_specular) * mix(vec3(1), vec3(0), roughness);
    vec3 rim_real = vec3(rim_intensity) * rim_amount_start.x;

    return diffuse_real + specular_real + rim_real;
}

void main() {
    vec2 uv = calculate_uv();

    vec4 col = vec4(calculate_lighting(), 1.0);
    if (col.a < 0.8) 
        discard;
    vec4 emissive = emissive_tint * vec4(texture(s_emissive, uv).rgb, 1.0);
    emissive.rgb *= emissive.a;
    fout_color = vec4(col.rgb + emissive.rgb + fin.color, 1.0);
    fout_id = uvec4(pc.selection_id, 0.0, 0.0, 0.0);
}