#include "particles.hpp"

#include "file.hpp"
#include "game.hpp"
#include "vuk/Partials.hpp"

namespace spellbook {

ParticleEmitter& instance_emitter(Scene* scene, const EmitterCPU& emitter_cpu) {
    static bool setup = false;

    if (!setup) {
        {
            vuk::PipelineBaseCreateInfo pci;
            pci.add_glsl(get_contents("src/shaders/particle_emitter.comp"), "particle_emitter.comp");
            game.renderer.context->create_named_pipeline("emitter", pci);
        }
        {
            vuk::PipelineBaseCreateInfo pci2;
            pci2.add_glsl(get_contents("src/shaders/particle.vert"), "particle.vert");
            pci2.add_glsl(get_contents("src/shaders/textured_3d.frag"), "textured_3d.frag");
            game.renderer.context->create_named_pipeline("particle", pci2);
        }
        setup = true;
    }

    ParticleEmitter emitter;
    emitter.settings.position_scale.xyz = emitter_cpu.position - 0.5f * emitter_cpu.position_random;
    emitter.settings.position_scale.w = emitter_cpu.scale - 0.5f * emitter_cpu.scale_random;
    emitter.settings.velocity_damping.xyz = emitter_cpu.velocity - 0.5f * emitter_cpu.velocity_random;
    emitter.settings.velocity_damping.w = emitter_cpu.damping;
    emitter.settings.life = emitter_cpu.duration - 0.5f * emitter_cpu.duration_random;

    emitter.settings.position_scale_random.xyz = emitter_cpu.position_random;
    emitter.settings.position_scale_random.w = emitter_cpu.scale_random;
    emitter.settings.velocity_damping_random.xyz = emitter_cpu.velocity_random;
    emitter.settings.life_random = emitter_cpu.duration_random;

    emitter.rate = 1.0f / emitter_cpu.particles_per_second;
    emitter.next_spawn = Input::time;
    
    MaterialCPU material_cpu = {
        .file_path = fmt_("{}_mat", emitter_cpu.file_path),
        .color_tint = palette::black,
        .shader_name = "particle"
    };
    emitter.material = game.renderer.upload_material(material_cpu);
    emitter.mesh = emitter_cpu.mesh;
    
    emitter.calculate_max();

    TextureCPU color_texture;
    color_texture.file_path = fmt_("{}_tex", emitter_cpu.file_path);
    color_texture.size = v2i(8, 8);
    color_texture.format = vuk::Format::eR8G8B8A8Srgb;
    color_texture.pixels.resize(color_texture.size.x * color_texture.size.y * 4);
    for (u32 y = 0; y < color_texture.size.y; ++y) {
        for (u32 x = 0; x < color_texture.size.x; ++x) {
            v2 f = v2(x, y) / v2(color_texture.size - v2i(1));
            Color color1 = mix(emitter_cpu.color1_start, emitter_cpu.color1_end, f.y);
            Color color2 = mix(emitter_cpu.color2_start, emitter_cpu.color2_end, f.y);
            Color color = mix(color1, color2, f.x);
            color_texture.pixels[(y * color_texture.size.x + x) * 4 + 0] = u8(color.r * 255.f);
            color_texture.pixels[(y * color_texture.size.x + x) * 4 + 1] = u8(color.g * 255.f);
            color_texture.pixels[(y * color_texture.size.x + x) * 4 + 2] = u8(color.b * 255.f);
            color_texture.pixels[(y * color_texture.size.x + x) * 4 + 3] = u8(color.a * 255.f);
        }
    }
    string tex_id = game.renderer.upload_texture(color_texture);
    emitter.color = vuk::make_sampled_image(game.renderer.get_texture_or_upload(tex_id).view.get(), Sampler().address(Address_Clamp).get());
    
    vector<u8> bytes;
    bytes.resize(emitter.settings.max_particles * sizeof(v4)*4 + 1);
    auto [buf, fut] = create_buffer_gpu(*game.renderer.global_allocator, vuk::DomainFlagBits::eTransferOnTransfer, std::span(bytes));
    emitter.particles_buffer = std::move(buf);
    game.renderer.enqueue_setup(std::move(fut));
    
    scene->render_scene.emitters.emplace_back(std::move(emitter));
    return scene->render_scene.emitters.last();
}

}
