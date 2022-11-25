#include "particles.hpp"

#include <vuk/Partials.hpp>

#include "lib/file.hpp"
#include "game/game.hpp"

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
    emitter.emitter_cpu = emitter_cpu;
    emitter.settings.position_scale.xyz = emitter_cpu.position - 0.5f * emitter_cpu.position_random;
    emitter.settings.position_scale.w = emitter_cpu.scale - 0.5f * emitter_cpu.scale_random;
    emitter.settings.velocity_damping.xyz = emitter_cpu.velocity - 0.5f * emitter_cpu.velocity_random;
    emitter.settings.velocity_damping.w = emitter_cpu.damping;
    emitter.settings.life = emitter_cpu.duration - 0.5f * emitter_cpu.duration_random;
    emitter.settings.falloff = emitter_cpu.falloff;

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
    
    emitter.update_color();
    emitter.update_size();
    
    scene->render_scene.emitters.emplace_back(std::move(emitter));
    return scene->render_scene.emitters.last();
}

void ParticleEmitter::update_color() {
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
    color = vuk::make_sampled_image(game.renderer.get_texture_or_upload(tex_id).view.get(), Sampler().address(Address_Clamp).get());
}

void ParticleEmitter::update_size() {
    calculate_max();

    vector<u8> bytes;
    bytes.resize(settings.max_particles * sizeof(v4)*4 + 1);
    auto [buf, fut] = create_buffer_gpu(*game.renderer.global_allocator, vuk::DomainFlagBits::eTransferOnTransfer, std::span(bytes));
    particles_buffer = std::move(buf);
    game.renderer.enqueue_setup(std::move(fut));
}

void inspect(ParticleEmitter* emitter) {
    bool size_changed = false;
    bool color_changed = false;
    ImGui::DragFloat3("Position Start", emitter->settings.position_scale.data, 0.02f);
    ImGui::DragFloat3("Position Range", emitter->settings.position_scale_random.data, 0.01f);
    
    ImGui::DragFloat("Scale", &emitter->settings.position_scale.a, 0.01f);
    ImGui::DragFloat("Scale Range", &emitter->settings.position_scale_random.a, 0.01f);

    ImGui::DragFloat3("Velocity Start", emitter->settings.velocity_damping.data, 0.02f);
    ImGui::DragFloat3("Velocity Range", emitter->settings.velocity_damping_random.data, 0.01f);

    ImGui::DragFloat("Damping", &emitter->settings.velocity_damping.a, 0.01f);
    ImGui::DragFloat("Damping Range", &emitter->settings.velocity_damping_random.a, 0.01f);
    
    ImGui::DragFloat("Falloff", &emitter->settings.falloff, 0.01f);
    size_changed |= ImGui::DragFloat("Life Start", &emitter->settings.life, 0.02f);
    size_changed |= ImGui::DragFloat("Life Range", &emitter->settings.life_random, 0.01f);
    size_changed |= ImGui::DragFloat("Rate", &emitter->rate, 0.01f, 0.001f, 0.0f, "%.6f");
    emitter->rate = math::max(emitter->rate, 0.00001f); 
    ImGui::InputText("Mesh", &emitter->mesh);

    color_changed |= ImGui::ColorEdit3("Color1 start", emitter->emitter_cpu.color1_start.data);
    color_changed |= ImGui::ColorEdit3("Color1 end", emitter->emitter_cpu.color1_end.data);
    color_changed |= ImGui::ColorEdit3("Color2 start", emitter->emitter_cpu.color2_start.data);
    color_changed |= ImGui::ColorEdit3("Color2 end", emitter->emitter_cpu.color2_end.data);

    if (size_changed)
        emitter->update_size();

    if (color_changed)
        emitter->update_color();
    
}



}
