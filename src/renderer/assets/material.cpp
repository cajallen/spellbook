#include "material.hpp"

#include <filesystem>
#include <imgui.h>
#include <vuk/Pipeline.hpp>

#include "asset_loader.hpp"
#include "console.hpp"
#include "game.hpp"

#include "renderer/renderer.hpp"
#include "renderer/samplers.hpp"


namespace spellbook {

void MaterialGPU::bind_parameters(vuk::CommandBuffer& cbuf) {
    *cbuf.map_scratch_buffer<MaterialDataGPU>(0, 3) = tints;
};
void MaterialGPU::bind_textures(vuk::CommandBuffer& cbuf) {
    assert_else(color.is_global && orm.is_global && normal.is_global && emissive.is_global);
    cbuf.bind_image(0, 4, color.global.iv).bind_sampler(0, 4, color.global.sci);
    cbuf.bind_image(0, 5, orm.global.iv).bind_sampler(0, 5, orm.global.sci);
    cbuf.bind_image(0, 6, normal.global.iv).bind_sampler(0, 6, normal.global.sci);
    cbuf.bind_image(0, 7, emissive.global.iv).bind_sampler(0, 7, emissive.global.sci);
};

void inspect(MaterialGPU* material) {
    ImGui::Text("Base Color");
    ImGui::Image(&*game.renderer.imgui_images.emplace(material->color), {100, 100});
    ImGui::ColorEdit4("Tint##BaseColor", material->tints.color_tint.data);
    
    ImGui::BeginGroup();
    ImGui::Text("ORM");
    ImGui::Image(&*game.renderer.imgui_images.emplace(material->orm), {100, 100});
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Normals");
    ImGui::Image(&*game.renderer.imgui_images.emplace(material->normal), {100, 100});
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Emissive");
    ImGui::Image(&*game.renderer.imgui_images.emplace(material->emissive), {100, 100});
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
    
    string ext = std::filesystem::path(material_cpu.file_path).extension().string();
    assert_else(ext == material_extension);
    
    file_dump(j, get_resource_path(material_cpu.file_path));
}

MaterialCPU load_material(const string& file_path) {
    string ext = std::filesystem::path(file_path).extension().string();
    assert_else(ext == material_extension);
    
    json j = parse_file(get_resource_path(file_path));
    auto material_cpu = from_jv<MaterialCPU>(to_jv(j));
    material_cpu.file_path = file_path;
    return material_cpu;
}


}

