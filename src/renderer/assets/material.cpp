#include "material.hpp"

#include <imgui.h>

#include "console.hpp"
#include "game.hpp"

#include "renderer/renderer.hpp"
#include "renderer/samplers.hpp"


namespace spellbook {

void MaterialGPU::bind_parameters(vuk::CommandBuffer& cbuf) {
    *cbuf.map_scratch_buffer<MaterialDataGPU>(0, 3) = tints;
};
void MaterialGPU::bind_textures(vuk::CommandBuffer& cbuf) {
    cbuf.bind_image(0, 4, base_color_view).bind_sampler(0, 4, TrilinearClamp);
    cbuf.bind_image(0, 5, metallic_roughness_view).bind_sampler(0, 5, TrilinearClamp);
    cbuf.bind_image(0, 6, normal_view).bind_sampler(0, 6, TrilinearClamp);
    cbuf.bind_image(0, 7, emissive_view).bind_sampler(0, 7, TrilinearClamp);
};

void inspect(MaterialGPU* material) {
    auto si_base_color = vuk::make_sampled_image(material->base_color_view, {});
    ImGui::Text("Base Color");
    ImGui::Image(&*game.renderer.sampled_images.emplace(si_base_color), {100, 100});
    ImGui::ColorEdit4("Tint##BaseColor", material->tints.base_color_tint.data);

    auto si_mr = vuk::make_sampled_image(material->metallic_roughness_view, {});
    auto si_n  = vuk::make_sampled_image(material->normal_view, {});
    auto si_e  = vuk::make_sampled_image(material->emissive_view, {});
    ImGui::BeginGroup();
    ImGui::Text("Metal/Rough");
    ImGui::Image(&*game.renderer.sampled_images.emplace(si_mr), {100, 100});
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Normals");
    ImGui::Image(&*game.renderer.sampled_images.emplace(si_n), {100, 100});
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Emissive");
    ImGui::Image(&*game.renderer.sampled_images.emplace(si_e), {100, 100});
    ImGui::EndGroup();
    ImGui::ColorEdit4("Emissive Tint", material->tints.emissive_tint.data);

    ImGui::DragFloat("Roughness Factor", &material->tints.roughness_metallic_normal_scale.x, 0.01f);
    ImGui::DragFloat("Metallic Factor", &material->tints.roughness_metallic_normal_scale.y, 0.01f);
    ImGui::DragFloat("Normal Factor", &material->tints.roughness_metallic_normal_scale.z, 0.01f);
    ImGui::DragFloat("UV Scale", &material->tints.roughness_metallic_normal_scale.w, 0.01f);

    s32 cull_mode = (s32) (u32) material->cull_mode;
    ImGui::Combo("Cull Mode", &cull_mode, "None\0Front\0Back\0Both\0");
    material->cull_mode = (vuk::CullModeFlags) cull_mode;
}

void save_material(const MaterialCPU& material_cpu) {
    auto j = from_jv<json>(to_jv(material_cpu));
    file_dump(j, material_cpu.file_name);
}

MaterialCPU load_material(const string_view file_name) {
    json j = parse_file(file_name);
    auto material_cpu = from_jv<MaterialCPU>(to_jv(j));
    material_cpu.file_name = file_name;
    return material_cpu;
}

u64 MaterialCPU::contents_hash() const {
    u64 hash1 = hash_data(&base_color_tint, sizeof(Color) * 2 + sizeof(f32) * 3);
    u64 hash2 = hash_data(base_color_texture.data(), base_color_texture.size());
    u64 hash3 = hash_data(orm_texture.data(), orm_texture.size());
    u64 hash4 = hash_data(normal_texture.data(), normal_texture.size());
    u64 hash5 = hash_data(emissive_texture.data(), emissive_texture.size());
    u64 hash6 = hash_data(&uv_scale, sizeof(uv_scale));
    u64 hash7 = hash_data(&cull_mode, sizeof(cull_mode));

    return hash1 ^ hash2 ^ hash3 ^ hash4 ^ hash5 ^ hash6 ^ hash7;
}


}

