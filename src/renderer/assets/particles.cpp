#include "particles.hpp"

#include <vuk/Partials.hpp>

#include "extension/fmt.hpp"
#include "general/file.hpp"
#include "game/game.hpp"
#include "renderer/render_scene.hpp"
namespace spellbook {

void setup_emitter() {
    static bool setup = false;

    if (!setup) {
        {
            vuk::PipelineBaseCreateInfo pci;
            pci.add_glsl(get_contents("src/shaders/particle_emitter.comp"), "src/shaders/particle_emitter.comp");
            game.renderer.context->create_named_pipeline("emitter", pci);
        }
        {
            vuk::PipelineBaseCreateInfo pci;
            pci.add_glsl(get_contents("src/shaders/particle.vert"), "src/shaders/particle.vert");
            pci.add_glsl(get_contents("src/shaders/textured_3d.frag"), "src/shaders/textured_3d.frag");
            game.renderer.context->create_named_pipeline("particle", pci);
        }
        setup = true;
    }
}

EmitterGPU& instance_emitter(RenderScene& scene, const EmitterCPU& emitter_cpu) {
    setup_emitter();
    
    EmitterGPU emitter;
    u64 mat_id = hash_data(emitter_cpu.material.data(), emitter_cpu.material.size());
    assert_else(game.renderer.material_cache.contains(mat_id));
    game.renderer.material_cache[mat_id].pipeline = game.renderer.context->get_named_pipeline("particle");
    emitter.update_from_cpu(emitter_cpu);
    
    scene.emitters.emplace_back(std::move(emitter));
    return scene.emitters.back();
}

void EmitterGPU::update_from_cpu(const EmitterCPU& emitter) {
    emitter_cpu = emitter;

    // Validate
    emitter_cpu.particles_per_second = math::max(FLT_MIN, emitter_cpu.particles_per_second);
    
    settings.position_scale.xyz = emitter_cpu.position - 0.5f * emitter_cpu.position_random;
    settings.position_scale.w = emitter_cpu.scale - 0.5f * emitter_cpu.scale_random;
    settings.velocity_damping.xyz = emitter_cpu.velocity - 0.5f * emitter_cpu.velocity_random;
    settings.velocity_damping.w = emitter_cpu.damping;
    settings.life = emitter_cpu.duration - 0.5f * emitter_cpu.duration_random;
    settings.falloff = emitter_cpu.falloff;

    settings.position_scale_random.xyz = emitter_cpu.position_random;
    settings.position_scale_random.w = emitter_cpu.scale_random;
    settings.velocity_damping_random.xyz = emitter_cpu.velocity_random;
    settings.life_random = emitter_cpu.duration_random;
    
    rate = 1.0f / emitter_cpu.particles_per_second;
    
    mesh = emitter_cpu.mesh;
    material = emitter_cpu.material;

    update_color();
    update_size();
}


void EmitterGPU::update_color() {
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
    color = vuk::make_sampled_image(game.renderer.get_texture_or_upload(tex_id).value.view.get(), Sampler().address(Address_Clamp).get());
}

void EmitterGPU::update_size() {
    calculate_max();

    vector<u8> bytes;
    bytes.resize(settings.max_particles * sizeof(v4)*4 + 1);
    auto [buf, fut] = create_buffer(*game.renderer.global_allocator, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(bytes));
    particles_buffer = std::move(buf);
    game.renderer.enqueue_setup(std::move(fut));
}

void inspect(EmitterCPU* emitter) {
    ImGui::DragFloat3("Position", emitter->position.data, 0.01f);
    ImGui::DragFloat3("Velocity", emitter->velocity.data, 0.01f);
    ImGui::DragFloat("Damping", &emitter->damping, 0.01f);
    ImGui::DragFloat("Scale", &emitter->scale, 0.01f);
    ImGui::DragFloat("Duration", &emitter->duration, 0.01f);
    ImGui::DragFloat("Falloff", &emitter->falloff, 0.01f);
    ImGui::DragFloat("Particles Per Second", &emitter->particles_per_second, 0.01f);
    ImGui::DragFloat3("Position Random", emitter->position_random.data, 0.01f);
    ImGui::DragFloat3("Velocity Random", emitter->velocity_random.data, 0.01f);
}

void inspect(EmitterGPU* emitter) {
    bool size_changed = false;
    bool color_changed = false;

    if (size_changed)
        emitter->update_size();

    if (color_changed)
        emitter->update_color();
}

void update_emitter(EmitterGPU& emitter, vuk::CommandBuffer& command_buffer) {
    struct PC {
        u32 spawn_count = 0;
        float dt;
    } pc;
    pc.spawn_count = math::max((Input::time - emitter.next_spawn) / emitter.rate, 0.0f);
    emitter.next_spawn += emitter.rate * pc.spawn_count;
                
    pc.dt = Input::delta_time;
    command_buffer
        .bind_compute_pipeline("emitter")
        .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, pc)
        .bind_buffer(0, 0, *emitter.particles_buffer);
    *command_buffer.map_scratch_buffer<EmitterSettings>(0, 1) = emitter.settings;
    command_buffer.dispatch(emitter.settings.max_particles); 
}

void render_particles(EmitterGPU& emitter, vuk::CommandBuffer& command_buffer) {
    MeshGPU* mesh = game.renderer.get_mesh(emitter.mesh);
    MaterialGPU* material = game.renderer.get_material(emitter.material);

    if (mesh == nullptr || material == nullptr) {
        log_error("Particles has missing mesh or material");
        return;
    }
    
    command_buffer // Mesh
        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
    command_buffer // Material
        .set_rasterization({.cullMode = material->cull_mode})
        .bind_buffer(0, PARTICLES_BINDING, *emitter.particles_buffer)
        .bind_graphics_pipeline(material->pipeline);
    command_buffer.bind_image(0, SPARE_BINDING_1, emitter.color.global.iv).bind_sampler(0, SPARE_BINDING_1, emitter.color.global.sci);
    material->bind_parameters(command_buffer);
    material->bind_textures(command_buffer);
    command_buffer.draw_indexed(mesh->index_count, emitter.settings.max_particles, 0, 0, 0);
}

}
