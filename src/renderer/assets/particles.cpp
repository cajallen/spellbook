#include "particles.hpp"

#include <vuk/Partials.hpp>

#include "mesh_asset.hpp"
#include "editor/console.hpp"
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
    
    return *scene.emitters.emplace(std::move(emitter));
}

void deinstance_emitter(EmitterGPU& emitter, bool wait_despawn) {
    if (wait_despawn) {
        emitter.rate = FLT_MAX;
        emitter.next_spawn = FLT_MAX;
        emitter.deinstance_at = math::min(emitter.deinstance_at, Input::time + emitter.settings.life + emitter.settings.life_random);
    } else {
        emitter.deinstance_at = Input::time;
    }
}

void EmitterGPU::update_from_cpu(const EmitterCPU& new_emitter) {
    settings.position_scale.xyz = new_emitter.position + new_emitter.offset - 0.5f * new_emitter.position_random;
    settings.position_scale.w = new_emitter.scale - 0.5f * new_emitter.scale_random;
    settings.velocity_damping.xyz = new_emitter.velocity - 0.5f * new_emitter.velocity_random;
    settings.velocity_damping.w = new_emitter.damping;
    settings.life = new_emitter.duration - 0.5f * new_emitter.duration_random;
    settings.falloff = new_emitter.falloff;

    settings.position_scale_random.xyz = new_emitter.position_random;
    settings.position_scale_random.w = new_emitter.scale_random;
    settings.velocity_damping_random.xyz = new_emitter.velocity_random;
    settings.life_random = new_emitter.duration_random;

    settings.alignment_vector = v4(new_emitter.alignment_vector - new_emitter.alignment_random, new_emitter.alignment_vector == v3(0.0f) ? 0.0f : 1.0f);
    settings.alignment_random = v4(new_emitter.alignment_random * 2.0, 0.0f);

    rate = 1.0f / new_emitter.particles_per_second;
    
    mesh = new_emitter.mesh;
    material = new_emitter.material;

    bool upload_color = false, upload_size = false;
    if (emitter_cpu.color1_start != new_emitter.color1_start ||
        emitter_cpu.color1_end != new_emitter.color1_end ||
        emitter_cpu.color2_start != new_emitter.color2_start ||
        emitter_cpu.color2_end != new_emitter.color2_end ||
        color.texture.empty()) {
        upload_color = true;
    }
    if (emitter_cpu.particles_per_second != new_emitter.particles_per_second ||
        emitter_cpu.duration != new_emitter.duration ||
        emitter_cpu.duration_random != new_emitter.duration_random ||
        !particles_buffer) {
        upload_size = true;
    }

    emitter_cpu = new_emitter;

    if (upload_color)
        update_color();
    if (upload_size)
        update_size();

    next_spawn = Input::time;
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

    u64 tex_id = hash_string(color_texture.file_path);
    if (game.renderer.texture_cache.contains(tex_id))
        game.renderer.texture_cache.erase(tex_id);
    upload_texture(color_texture);
    color = {color_texture.file_path, Sampler().address(Address_Clamp)};
}

void EmitterGPU::update_size() {
    calculate_max();

    vector<u8> bytes;
    bytes.resize(settings.max_particles * sizeof(v4)*4 + 4);
    auto [buf, fut] = create_buffer(*game.renderer.global_allocator, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(bytes));
    particles_buffer = std::move(buf);
    game.renderer.enqueue_setup(std::move(fut));
}

bool inspect(EmitterCPU* emitter) {
    bool changed = false;
    ImGui::PathSelect("File", &emitter->file_path, "resources", FileType_Emitter);
    changed |= ImGui::DragFloat3("Offset", emitter->offset.data, 0.01f);
    changed |= ImGui::DragFloat3("Offset Random", emitter->position_random.data, 0.01f);
    changed |= ImGui::DragFloat3("Velocity", emitter->velocity.data, 0.01f);
    changed |= ImGui::DragFloat3("Velocity Random", emitter->velocity_random.data, 0.01f);
    changed |= ImGui::DragFloat("Damping", &emitter->damping, 0.01f);

    changed |= ImGui::DragFloat3("Alignment", emitter->alignment_vector.data, 0.01f);
    changed |= ImGui::DragFloat3("Alignment Random", emitter->alignment_random.data, 0.01f);

    changed |= ImGui::DragFloat("Scale", &emitter->scale, 0.01f);
    changed |= ImGui::DragFloat("Scale Random", &emitter->scale_random, 0.01f);
    changed |= ImGui::DragFloat("Particles Per Second", &emitter->particles_per_second, 0.01f);
    changed |= ImGui::DragFloat("Duration", &emitter->duration, 0.01f);
    changed |= ImGui::DragFloat("Duration Random", &emitter->duration_random, 0.01f);
    changed |= ImGui::DragFloat("Falloff", &emitter->falloff, 0.01f);

    changed |= ImGui::ColorEdit3("Color 1 Start", emitter->color1_start.data);
    changed |= ImGui::ColorEdit3("Color 1 End", emitter->color1_end.data);
    changed |= ImGui::ColorEdit3("Color 2 Start", emitter->color2_start.data);
    changed |= ImGui::ColorEdit3("Color 2 End", emitter->color2_end.data);
    
    changed |= ImGui::PathSelect("Mesh", &emitter->mesh, "resources/models", FileType_Mesh);

    return changed;
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

    u64 tex_id = hash_string(emitter.color.texture);
    TextureGPU& tex = game.renderer.texture_cache[tex_id];
    command_buffer.bind_image(0, SPARE_BINDING_1, tex.value.view.get()).bind_sampler(0, SPARE_BINDING_1, emitter.color.sampler.get());
    material->bind_parameters(command_buffer);
    material->bind_textures(command_buffer);
    command_buffer.draw_indexed(mesh->index_count, emitter.settings.max_particles, 0, 0, 0);
}

void upload_dependencies(EmitterGPU& renderable) {
    MeshGPU* mesh = game.renderer.get_mesh(renderable.mesh);
    MaterialGPU* material = game.renderer.get_material(renderable.material);

    if (renderable.mesh.empty() || renderable.material.empty())
        return;
    if (mesh == nullptr) {
        if (exists(to_resource_path(renderable.mesh))) {
            upload_mesh(load_mesh(renderable.mesh));
        } else {
            console({.str = "Renderable mesh asset not found: " + renderable.mesh, .group = "assets", .frame_tags = {"render_scene"}});
        }
    }
    if (material == nullptr) {
        if (exists(to_resource_path(renderable.material))) {
            upload_material(load_material(renderable.material));
        } else {
            console({.str = "Renderable material asset not found: " + renderable.material, .group = "assets", .frame_tags = {"render_scene"}});
        }
    }
}

}
