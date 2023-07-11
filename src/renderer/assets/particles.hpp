#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/SampledImage.hpp>

#include "general/color.hpp"
#include "general/string.hpp"
#include "general/json.hpp"
#include "general/matrix.hpp"
#include "general/quaternion.hpp"
#include "renderer/vertex.hpp"
#include "renderer/image.hpp"


namespace spellbook {
struct Scene;
struct RenderScene;

struct EmitterCPU {
    string file_path;
    vector<string> dependencies;
    
    v3 offset = v3(0.0f);
    v3 position = v3(0.0f);
    v3 velocity = v3(0.0f);
    quat rotation = quat();
    float damping = 1.0f;
    float scale = 1.0f;
    float duration = 1.0f;
    float falloff = 0.5f;

    float particles_per_second = 1.0f;
    
    Color color1_start = palette::white;
    Color color1_end = palette::white;
    Color color2_start = palette::white;
    Color color2_end = palette::white;
    
    v3 velocity_random = v3(0.0f);
    v3 position_random = v3(0.0f);
    float scale_random = 0.0f;
    float duration_random = 0.0f;
    float damping_random = 0.0f;

    v3 alignment_vector = v3(0.0f);
    v3 alignment_random = v3(0.0f);

    string mesh;
    string material;

    void set_velocity_direction(v3 dir);
};

struct EmitterSettings {
    m44GPU pose_matrix = (m44GPU) m44::identity();
    v4 velocity_damping;

    v4 scale_unused;

    v4 position_scale_random;
    v4 velocity_damping_random;

    v4 alignment_vector;
    v4 alignment_random;

    float life;
    float life_random;
    float falloff;
    
    u32 max_particles;
};

struct EmitterGPU {
    static constexpr u64 cube_mesh = hash_view("emitter_cube");
    static constexpr u64 sphere_mesh = hash_view("emitter_sphere");
    // For editing purposes
    EmitterCPU emitter_cpu;
    
    EmitterSettings settings;
    float rate;
    float next_spawn;
    
    Image color;
    vuk::Unique<vuk::Buffer> particles_buffer;
    u64 mesh;
    u64 material;

    bool emitting = true;
    float deinstance_at = FLT_MAX;

    void calculate_max() {
        settings.max_particles = (settings.life + settings.life_random) / rate + 1;
    }

    void update_from_cpu(const EmitterCPU& new_emitter);
    void update_color();
    void update_size();
};

EmitterGPU& instance_emitter(RenderScene& scene, const EmitterCPU& emitter_cpu);
void deinstance_emitter(EmitterGPU& emitter, bool wait_despawn = true);

bool inspect(Scene* scene, EmitterCPU* emitter);

void update_emitter(EmitterGPU& emitter, vuk::CommandBuffer& command_buffer);
void render_particles(EmitterGPU& emitter, vuk::CommandBuffer& command_buffer);

void upload_dependencies(EmitterGPU& emitter);

JSON_IMPL(EmitterCPU, offset, velocity, damping, scale, duration, falloff, particles_per_second, color1_start, color1_end, color2_start, color2_end, velocity_random, position_random, scale_random, duration_random, alignment_vector, alignment_random, mesh, material);

}
