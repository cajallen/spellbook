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


vec2 calculate_uv() {
    return fin.uv * roughness_metallic_normals_scale.w;
    float scale = 20.0;
    vec2 st = fin.uv * scale;
        
    vec2 i_st = floor(st);
    vec2 f_st = fract(st);
    float m_dist = 1000.0;  // minimum distance
    vec2 uv = st;
    for(int y = -1; y <= 1; ++y){
        for(int x = -1; x <= 1; ++x){
        	vec2 neighbor = vec2(float(x),float(y));
            vec2 point = random2(i_st + neighbor);
            point = 0.5 + 0.5*sin(6.2831*point);
            vec2 diff = neighbor + point - f_st;
            float dist = length(diff);

            // Keep the closer distance
            if(dist < m_dist){
            	m_dist = dist;
                uv = st + diff;
            }
        }
    }
    return uv / scale;
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
    
    float toon_diffuse = smoothstep(0.0, 0.03, NdotL);
    
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