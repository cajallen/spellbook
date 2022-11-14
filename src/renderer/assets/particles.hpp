#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "color.hpp"
#include "vector.hpp"
#include "string.hpp"
#include "json.hpp"

#include "renderer/vertex.hpp"
#include "vuk/SampledImage.hpp"


namespace spellbook {
struct Scene;

struct EmitterCPU {
    string file_path;
    
    v3 position = v3(0.0f);
    v3 velocity = v3(0.0f);
    float damping = 1.0;
    float scale = 1.0;
    float duration = 1.0;

    float particles_per_second = 1.0f;
    
    Color color1_start = palette::white;
    Color color1_end = palette::white;
    Color color2_start = palette::white;
    Color color2_end = palette::white;
    
    v3 velocity_random = v3(0.0f);
    v3 position_random = v3(0.0f);
    float scale_random = 0.0f;
    float duration_random = 0.0f;

    string mesh;
};

struct ParticleEmitterSettings {
    v4 position_scale;
    v4 velocity_damping;

    v4 position_scale_random;
    v4 velocity_damping_random;
    
    float life;
    float life_random;
    
    u32 max_particles;
};

struct ParticleEmitter {
    ParticleEmitterSettings settings;
    float rate;
    float next_spawn;
    
    vuk::SampledImage color = vuk::SampledImage(vuk::SampledImage::Global{});
    vuk::Unique<vuk::BufferGPU> particles_buffer;
    string mesh;
    string material;

    void calculate_max() {
        settings.max_particles = (settings.life + settings.life_random) / rate + 1;
    }
};

ParticleEmitter& instance_emitter(Scene* scene, const EmitterCPU& emitter_cpu);


}
