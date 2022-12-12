#version 450
#pragma shader_stage(vertex)

layout (binding = 0) uniform VP {
	mat4 view;
	mat4 projection;
    vec4 camera_position;
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) out vec4 vout_position;

void main() {
    vec3 position;
    float extent = 100.0;
    switch(gl_VertexIndex) {
        case 0: { position = vec3(-extent, -extent,  0.0); vout_position = vec4(position, 1.0); } break;
        case 1: { position = vec3( extent,  extent,  0.0); vout_position = vec4(position, 1.0); } break;
        case 2: { position = vec3( extent, -extent,  0.0); vout_position = vec4(position, 1.0); } break;
        case 3: { position = vec3(-extent, -extent,  0.0); vout_position = vec4(position, 1.0); } break;
        case 4: { position = vec3( extent,  extent,  0.0); vout_position = vec4(position, 1.0); } break;
        case 5: { position = vec3(-extent,  extent,  0.0); vout_position = vec4(position, 1.0); } break;
    }

    gl_Position = projection * view * vec4(position, 1.0);
}
