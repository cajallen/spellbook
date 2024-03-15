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


void main() {
	vec4 total_position = vec4(0.0);
	
	int bones_used = 0;
	for (int i = 0 ; i < 4 ; i++) {
		if (vin_bone_id[i] == -1)
			continue;
		bones_used++;

		vec4 local_position = bones[vin_bone_id[i]] * vec4(vin_position, 1.0);
		total_position += local_position * vin_bone_weight[i];
	}

	total_position = bones_used > 0 ? total_position : vec4(vin_position, 1.0);

    vec4 h_position = model[gl_InstanceIndex] * total_position;
    gl_Position = vp * h_position;
}
