#include "particles.hpp"

#include <vuk/Partials.hpp>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "general/math/matrix_math.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/gpu_asset_cache.hpp"

namespace spellbook {

void setup_emitter() {
    static bool setup = false;

    if (!setup) {
        {
            vuk::PipelineBaseCreateInfo pci;
            pci.add_glsl(get_contents("shaders/particle_emitter.comp"_distributed), "shaders/particle_emitter.comp"_distributed.abs_string());
            get_renderer().context->create_named_pipeline("emitter", pci);
        }
        {
            vuk::PipelineBaseCreateInfo pci;
            pci.add_glsl(get_contents("shaders/particle.vert"_distributed), "shaders/particle.vert"_distributed.abs_string());
            pci.add_glsl(get_contents("shaders/textured_3d.frag"_distributed), "shaders/textured_3d.frag"_distributed.abs_string());
            get_renderer().context->create_named_pipeline("particle", pci);
        }

        auto cube_mesh = generate_cube(v3(0.0f), v3(1.0f));
        cube_mesh.file_path = FilePath(EmitterGPU::cube_mesh, FilePathLocation_Symbolic);
        auto sphere_mesh = generate_icosphere(2);
        sphere_mesh.file_path = FilePath(EmitterGPU::sphere_mesh, FilePathLocation_Symbolic);
        upload_mesh(cube_mesh);
        upload_mesh(sphere_mesh);
        setup = true;
    }
}

EmitterGPU& instance_emitter(RenderScene& scene, const EmitterCPU& emitter_cpu, float current_time) {
    setup_emitter();
    
    EmitterGPU emitter;
    uint64 mat_id = hash_path(emitter_cpu.material);
    get_gpu_asset_cache().paths[mat_id] = emitter_cpu.material;
    assert_else(get_gpu_asset_cache().materials.contains(mat_id));
    get_gpu_asset_cache().materials.at(mat_id).pipeline = get_renderer().context->get_named_pipeline("particle");
    assert_else(get_gpu_asset_cache().materials.at(mat_id).pipeline != nullptr);
    emitter.update_from_cpu(emitter_cpu, current_time);
    
    return *scene.emitters.emplace(std::move(emitter));
}

void deinstance_emitter(EmitterGPU& emitter, float current_time, bool wait_despawn) {
    if (wait_despawn) {
        emitter.rate = FLT_MAX;
        emitter.next_spawn = FLT_MAX;
        emitter.deinstance_at = math::min(emitter.deinstance_at, current_time + emitter.settings.life + emitter.settings.life_random);
    } else {
        emitter.deinstance_at = current_time;
    }
}

void EmitterGPU::update_from_cpu(const EmitterCPU& new_emitter, float current_time) {
    settings.pose_matrix = m44GPU(
        math::translate(new_emitter.position) *
        math::rotation(new_emitter.rotation) *
        math::translate(new_emitter.offset - 0.5f * new_emitter.position_random)
    );
    settings.scale_unused.x = new_emitter.scale - 0.5f * new_emitter.scale_random;
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

    mesh = hash_path(new_emitter.mesh);
    material = hash_path(new_emitter.material);
    get_gpu_asset_cache().paths[mesh] = new_emitter.mesh;
    get_gpu_asset_cache().paths[material] = new_emitter.material;

    bool upload_color = false, upload_size = false;
    if (emitter_cpu.color1_start != new_emitter.color1_start ||
        emitter_cpu.color1_end != new_emitter.color1_end ||
        emitter_cpu.color2_start != new_emitter.color2_start ||
        emitter_cpu.color2_end != new_emitter.color2_end ||
        !color.texture.is_file()) {
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

    if (next_spawn <= 0.0f)
        next_spawn = current_time;
}


void EmitterGPU::update_color() {
    TextureCPU color_texture;
    color_texture.file_path = FilePath(fmt_("{}_tex", emitter_cpu.file_path.rel_string()), FilePathLocation_Symbolic);
    color_texture.size = v2i(8, 8);
    color_texture.format = vuk::Format::eR8G8B8A8Srgb;
    color_texture.pixels.resize(color_texture.size.x * color_texture.size.y * 4);
    for (uint32 y = 0; y < color_texture.size.y; ++y) {
        for (uint32 x = 0; x < color_texture.size.x; ++x) {
            v2 f = v2(x, y) / v2(color_texture.size - v2i(1));
            Color color1 = mix(emitter_cpu.color1_start, emitter_cpu.color1_end, f.y);
            Color color2 = mix(emitter_cpu.color2_start, emitter_cpu.color2_end, f.y);
            Color color = mix(color1, color2, f.x);
            color_texture.pixels[(y * color_texture.size.x + x) * 4 + 0] = uint8(color.r * 255.f);
            color_texture.pixels[(y * color_texture.size.x + x) * 4 + 1] = uint8(color.g * 255.f);
            color_texture.pixels[(y * color_texture.size.x + x) * 4 + 2] = uint8(color.b * 255.f);
            color_texture.pixels[(y * color_texture.size.x + x) * 4 + 3] = uint8(color.a * 255.f);
        }
    }

    uint64 tex_id = hash_path(color_texture.file_path);
    get_gpu_asset_cache().paths[tex_id] = color_texture.file_path;
    if (get_gpu_asset_cache().textures.contains(tex_id))
        get_gpu_asset_cache().textures.erase(tex_id);
    upload_texture(color_texture);
    color = {color_texture.file_path, Sampler().address(Address_Clamp)};
}

void EmitterGPU::update_size() {
    calculate_max();

    vector<uint8> bytes;
    bytes.resize(settings.max_particles * sizeof(v4)*4 + 4);
    auto [buf, fut] = create_buffer(*get_renderer().global_allocator, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnTransfer, std::span(bytes));
    particles_buffer = std::move(buf);
    get_renderer().enqueue_setup(std::move(fut));
}

bool inspect(RenderScene& scene, EmitterCPU* emitter) {
    bool changed = false;
    ImGui::PathSelect<EmitterCPU>("File", &emitter->file_path);

    changed |= inspect_dependencies(emitter->dependencies, emitter->file_path);

    changed |= ImGui::DragFloat3("Offset", emitter->offset.data, 0.01f);
    changed |= ImGui::DragFloat3("Offset Random", emitter->position_random.data, 0.01f);
    changed |= ImGui::DragFloat3("Velocity", emitter->velocity.data, 0.01f);
    changed |= ImGui::DragFloat3("Velocity Random", emitter->velocity_random.data, 0.01f);
    changed |= ImGui::DragFloat("Damping", &emitter->damping, 0.01f);
    changed |= ImGui::DragFloat("Damping Random", &emitter->damping_random, 0.01f);

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

    // TODO
    //changed |= ImGui::PathSelect("Mesh", &emitter->mesh, FileType_Mesh);

    return changed;
}

void update_emitter(EmitterGPU& emitter, vuk::CommandBuffer& command_buffer, float current_time, float delta_time) {
    struct PC {
        uint32 spawn_count = 0;
        float dt;
    } pc;
    pc.spawn_count = math::max((current_time - emitter.next_spawn) / emitter.rate, 0.0f);
    emitter.next_spawn += emitter.rate * pc.spawn_count;
    if (!emitter.emitting)
        pc.spawn_count = 0;
                
    pc.dt = delta_time;
    command_buffer
        .bind_compute_pipeline("emitter")
        .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, pc)
        .bind_buffer(0, 0, *emitter.particles_buffer);
    *command_buffer.map_scratch_buffer<EmitterSettings>(0, 1) = emitter.settings;
    command_buffer.dispatch(emitter.settings.max_particles); 
}

void render_particles(EmitterGPU& emitter, vuk::CommandBuffer& command_buffer) {
    MeshGPU* mesh = get_gpu_asset_cache().get_mesh(emitter.mesh);
    MaterialGPU* material = get_gpu_asset_cache().get_material(emitter.material);

    if (mesh == nullptr || material == nullptr) {
        return;
    }
    
    command_buffer // Mesh
        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
    command_buffer // Material
        .set_rasterization({.cullMode = material->cull_mode})
        .bind_buffer(0, PARTICLES_BINDING, *emitter.particles_buffer)
        .bind_graphics_pipeline(material->pipeline);

    uint64 tex_id = hash_path(emitter.color.texture);
    get_gpu_asset_cache().paths[tex_id] = emitter.color.texture;
    TextureGPU& tex = get_gpu_asset_cache().textures[tex_id];
    command_buffer.bind_image(0, SPARE_BINDING_1, tex.value.view.get()).bind_sampler(0, SPARE_BINDING_1, emitter.color.sampler.get());
    material->bind_parameters(command_buffer);
    material->bind_textures(command_buffer);
    command_buffer.draw_indexed(mesh->index_count, emitter.settings.max_particles, 0, 0, 0);
}

void upload_dependencies(EmitterGPU& renderable) {
    if (renderable.mesh == 0 || renderable.material == 0)
        return;
    get_gpu_asset_cache().get_mesh_or_upload(renderable.mesh);
    get_gpu_asset_cache().get_material_or_upload(renderable.material);
}

void EmitterCPU::set_velocity_direction(v3 dir) {
    velocity = math::length(velocity) * math::normalize(dir);
}


}
