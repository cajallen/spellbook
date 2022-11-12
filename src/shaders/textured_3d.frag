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
    vec4 rim_alpha_width_start;
};
layout(binding = 3) uniform MaterialData {
	vec4 base_color_tint; 
	vec4 emissive_tint;
    vec4 roughness_metallic_normals_scale;
    vec4 emissive_dot_smoothstep;
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

vec3 linear_to_srgb(vec3 linear_rgb) {
    bvec3 cutoff = lessThan(linear_rgb, vec3(0.0031308));
    vec3  higher = vec3(1.055) * pow(linear_rgb, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3  lower	 = linear_rgb * vec3(12.92);

    return mix(higher, lower, cutoff);
}

// Converts a color from sRGB gamma to linear light gamma
vec3 srgb_to_linear(vec3 sRGB) {
    bvec3 cutoff = lessThan(sRGB, vec3(0.04045));
    vec3  higher = pow((sRGB + vec3(0.055)) / vec3(1.055), vec3(2.4));
    vec3  lower	 = sRGB / vec3(12.92);

    return mix(higher, lower, cutoff);
}

vec2 calculate_uv() {
    return fin.uv * roughness_metallic_normals_scale.w;
}

float calculate_toon_diffuse(float NdotL) {
    const float transition = 0.02;
    const float transition1_start = -0.25;
    const float transition2_start = -transition1_start - transition;
    const float range = 0.15;
    const float mid_out = 0.5;

    float val = 0.0;
    if (NdotL < transition1_start) {
        float out_min = 0.0;
        float out_max = range;
        val = map(NdotL, -1.0, transition1_start, out_min, out_max);
    }
    else if (NdotL < transition1_start + transition) {
        // transition1
        float out_min = range;
        float out_max = mid_out - 0.5 * range;
        val = map(NdotL, transition1_start, transition1_start + transition, out_min, out_max);
    }
    else if (NdotL < transition2_start) {
        // middle
        float out_min = mid_out - 0.5 * range;
        float out_max = mid_out + 0.5 * range;
        val = map(NdotL, transition1_start + transition, transition2_start, out_min, out_max);
    }
    else if (NdotL < transition2_start + transition) {
        // transition2
        float out_min = mid_out + 0.5 * range;
        float out_max = 1.0 - range;
        val = map(NdotL, transition2_start, transition2_start + transition, out_min, out_max);
    }
    else {
        // high
        float out_min = 1.0 - range;
        float out_max = 1.0;
        val = map(NdotL, transition2_start + transition, 1.0, out_min, out_max);
    }
    return val;
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
    vec3 reflection_dir = reflect(to_light, normal);
    float NdotL = dot(normal, to_light);
    float VdotR = dot(view_dir, reflection_dir);
    float VdotN = dot(view_dir, normal);

    vec4 albedo = texture(s_base_color, uv) * base_color_tint;
    albedo.rgb = albedo.rgb;

    vec4 emissive_tex = emissive_tint * vec4(texture(s_emissive, uv).rgb, 1.0);
    vec3 emissive_vert = fin.color;
    emissive_tex.rgb = emissive_tex.rgb;
    vec3 emissive = emissive_tex.rgb * emissive_tex.a + emissive_vert;
    emissive *= smoothstep(emissive_dot_smoothstep.x, emissive_dot_smoothstep.y, NdotL);
    
    float roughness = texture(s_metallic_roughness, uv).g * roughness_metallic_normals_scale.x;
    float smoothness = clamp(1.0 - roughness, 0.001, 1.0);

    float toon_diffuse = calculate_toon_diffuse(NdotL);
    float specular_dot = pow(clamp(VdotR, 0.0, 1.0), smoothness * 100.0);
    float toon_specular = smoothstep(0.4, 0.5, specular_dot);

    float rim_dot = max(1.0 + VdotN, 0.0);
    float rim_intensity = rim_dot * pow(NdotL, rim_alpha_width_start.y);
    rim_intensity = smoothstep(rim_alpha_width_start.z - 0.01, rim_alpha_width_start.z + 0.01, rim_intensity);

    vec3 diffuse = albedo.rgb * clamp(toon_diffuse, 0.0, 1.0) + albedo.rgb * ambient.rgb * ambient.a;
    vec3 specular = vec3(toon_specular) * mix(vec3(1.0), vec3(0.0), pow(roughness, 5.0));
    vec3 rim = vec3(rim_intensity) * rim_alpha_width_start.x;
    
    return diffuse + specular + rim + emissive;
}

void main() {
    vec3 col = calculate_lighting();
    fout_color = vec4(col, 1.0);
    fout_id = uvec4(pc.selection_id, 0.0, 0.0, 0.0);
}