#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "color.hpp"
#include "vector.hpp"
#include "string.hpp"
#include "json.hpp"

#include "renderer/vertex.hpp"


namespace spellbook {

struct ParticleEmitterSettings {
    v4 position_scale;
    v4 velocity_damping;
    v4 color;

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
    
    vuk::Unique<vuk::BufferGPU> particles_buffer;
    string mesh;
    string material;

    void calculate_max() {
        settings.max_particles = (settings.life + settings.life_random) / rate + 1;
    }
};


}