#include "material.hpp"

#include <filesystem>
#include <imgui.h>
#include <vuk/Pipeline.hpp>

#include "lib_ext/imgui_extra.hpp"

#include "asset_loader.hpp"
#include "console.hpp"
#include "game.hpp"
#include "file.hpp"
#include "game/asset_browser.hpp"

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

void inspect(MaterialCPU* material) {
    ImGui::PathSelect("File", &material->file_path, "resources", FileType_Material);

    ImGui::ColorEdit4("color_tint", material->color_tint.data, ImGuiColorEditFlags_DisplayHSV);
    ImGui::ColorEdit4("emissive_tint", material->emissive_tint.data, ImGuiColorEditFlags_DisplayHSV);
    ImGui::DragFloat("roughness_factor", &material->roughness_factor, 0.01f);
    ImGui::DragFloat("metallic_factor", &material->metallic_factor, 0.01f);
    ImGui::DragFloat("normal_factor", &material->normal_factor, 0.01f);
    ImGui::DragFloat2("emissive_dot_smoothstep", material->emissive_dot_smoothstep.data, 0.01f);

    ImGui::PathSelect("color_asset_path", &material->color_asset_path, "resources", FileType_Texture);
    ImGui::PathSelect("orm_asset_path", &material->orm_asset_path, "resources", FileType_Texture);
    ImGui::PathSelect("normal_asset_path", &material->normal_asset_path, "resources", FileType_Texture);
    ImGui::PathSelect("emissive_asset_path", &material->emissive_asset_path, "resources", FileType_Texture);

    ImGui::EnumCombo("cull_mode", &material->cull_mode);
}

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
    
    ImGui::DragFloat2("Emissive Direction", material->tints.emissive_dot_smoothstep.data, 0.01f);
}

void save_material(const MaterialCPU& material_cpu) {
    auto j = from_jv<json>(to_jv(material_cpu));
    
    string ext = std::filesystem::path(material_cpu.file_path).extension().string();
    assert_else(ext == extension(FileType_Material));
    
    file_dump(j, to_resource_path(material_cpu.file_path).string());
}

MaterialCPU load_material(const string& file_path) {
    string ext = std::filesystem::path(file_path).extension().string();
    assert_else(ext == extension(FileType_Material));
    
    json j = parse_file(to_resource_path(file_path).string());
    auto material_cpu = from_jv<MaterialCPU>(to_jv(j));
    material_cpu.file_path = file_path;
    return material_cpu;
}


}

