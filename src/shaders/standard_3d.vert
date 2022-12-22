#version 460
#pragma shader_stage(vertex)

#include "include.glsli"

layout (location = 0) in vec3 vin_position;
layout (location = 1) in vec3 vin_normal;
layout (location = 2) in vec3 vin_tangent;
layout (location = 3) in vec3 vin_color;
layout (location = 4) in vec2 vin_uv;
layout (location = 5) in ivec4 vin_bone_id;
layout (location = 6) in vec4 vin_bone_weight;

layout (binding = CAMERA_BINDING) uniform CameraData {
	mat4 vp;
};

layout (binding = MODEL_BINDING) buffer readonly Model {
	mat4 model[];
};
layout (binding = BONES_BINDING) buffer readonly Bones {
	int bone_count;
	mat4 bones[];
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
    vec3 position;
    vec3 color;
    vec2 uv;
    mat3 TBN;
} vout;


void main() {
	vec4 total_position = vec4(0.0);
	vec3 total_normal = vec3(0.0);
	vec3 total_tangent = vec3(0.0);
	
	int bones_used = 0;
	for (int i = 0 ; i < 4 ; i++) {
		if (vin_bone_id[i] == -1)
			continue;
		bones_used++;

		vec4 local_position = bones[vin_bone_id[i]] * vec4(vin_position, 1.0);
		total_position += local_position * vin_bone_weight[i];
		
		vec3 local_normal = mat3(bones[vin_bone_id[i]]) * vin_normal;
		vec3 local_tangent = mat3(bones[vin_bone_id[i]]) * vin_tangent;
		total_normal += local_normal;
		total_tangent += local_tangent;
	}

	total_position = bones_used > 0 ? total_position : vec4(vin_position, 1.0);
	total_normal = bones_used > 0 ? total_normal : vin_normal;
	total_tangent = bones_used > 0 ? total_tangent : vin_tangent;
	
    vec4 h_position = model[gl_BaseInstance] * total_position;
    vout.position = h_position.xyz / h_position.w;
	
    mat3 N = inverse(transpose(mat3(model[gl_BaseInstance])));
	vec3 n = normalize(N * normalize(total_normal));
	vec3 t = normalize(N * normalize(total_tangent));
	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);
	vout.TBN      = mat3(t, b, n);
    
	vout.uv = vin_uv;
	vout.color = vin_color;
    gl_Position = vp * h_position;
}
