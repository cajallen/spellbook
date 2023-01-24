#include "material.hpp"

#include <filesystem>
#include <imgui.h>

#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/game.hpp"
#include "renderer/renderer.hpp"


namespace spellbook {

void MaterialGPU::bind_parameters(vuk::CommandBuffer& cbuf) {
    *cbuf.map_scratch_buffer<MaterialDataGPU>(0, MATERIAL_BINDING) = tints;
};
void MaterialGPU::bind_textures(vuk::CommandBuffer& cbuf) {
    assert_else(color.is_global && orm.is_global && normal.is_global && emissive.is_global);
    cbuf.bind_image(0, BASE_COLOR_BINDING, color.global.iv).bind_sampler(0, BASE_COLOR_BINDING, color.global.sci);
    cbuf.bind_image(0, ORM_BINDING, orm.global.iv).bind_sampler(0, ORM_BINDING, orm.global.sci);
    cbuf.bind_image(0, NORMAL_BINDING, normal.global.iv).bind_sampler(0, NORMAL_BINDING, normal.global.sci);
    cbuf.bind_image(0, EMISSIVE_BINDING, emissive.global.iv).bind_sampler(0, EMISSIVE_BINDING, emissive.global.sci);
};

string upload_material(const MaterialCPU& material_cpu, bool frame_allocation) {
    assert_else(!material_cpu.file_path.empty());
    u64 material_cpu_hash               = hash_data(material_cpu.file_path.data(), material_cpu.file_path.size());

    MaterialGPU material_gpu;
    material_gpu.material_cpu = material_cpu;
    material_gpu.frame_allocated = frame_allocation;
    material_gpu.pipeline      = game.renderer.context->get_named_pipeline(vuk::Name(material_cpu.shader_name));
    material_gpu.color = vuk::make_sampled_image(game.renderer.get_texture_or_upload(material_cpu.color_asset_path).value.view.get(), material_cpu.sampler.get());
    material_gpu.normal = vuk::make_sampled_image(game.renderer.get_texture_or_upload(material_cpu.normal_asset_path).value.view.get(), material_cpu.sampler.get());
    material_gpu.orm = vuk::make_sampled_image(game.renderer.get_texture_or_upload(material_cpu.orm_asset_path).value.view.get(), material_cpu.sampler.get());
    material_gpu.emissive = vuk::make_sampled_image(game.renderer.get_texture_or_upload(material_cpu.emissive_asset_path).value.view.get(), material_cpu.sampler.get());

    material_gpu.tints         = {
        (v4) material_cpu.color_tint,
        (v4) material_cpu.emissive_tint,
        {material_cpu.roughness_factor, material_cpu.metallic_factor, material_cpu.normal_factor, 1.0f},
    };
    material_gpu.cull_mode = material_cpu.cull_mode;
    material_gpu.frame_allocated = frame_allocation;

    game.renderer.material_cache[material_cpu_hash] = std::move(material_gpu);
    return material_cpu.file_path;
}

void MaterialGPU::update_from_cpu(const MaterialCPU& new_material) {
    pipeline      = game.renderer.context->get_named_pipeline(vuk::Name(new_material.shader_name));
    tints         = {
        (v4) new_material.color_tint,
        (v4) new_material.emissive_tint,
        {new_material.roughness_factor, new_material.metallic_factor, new_material.normal_factor, 1.0f},
    };
    cull_mode = new_material.cull_mode;

    if (material_cpu.color_asset_path != new_material.color_asset_path)
        color = vuk::make_sampled_image(game.renderer.get_texture_or_upload(new_material.color_asset_path).value.view.get(), new_material.sampler.get());
    if (material_cpu.normal_asset_path != new_material.normal_asset_path)
        normal = vuk::make_sampled_image(game.renderer.get_texture_or_upload(new_material.normal_asset_path).value.view.get(), new_material.sampler.get());
    if (material_cpu.orm_asset_path != new_material.orm_asset_path)
        orm = vuk::make_sampled_image(game.renderer.get_texture_or_upload(new_material.orm_asset_path).value.view.get(), new_material.sampler.get());
    if (material_cpu.emissive_asset_path != new_material.emissive_asset_path)
        emissive = vuk::make_sampled_image(game.renderer.get_texture_or_upload(new_material.emissive_asset_path).value.view.get(), new_material.sampler.get());

    material_cpu = new_material;
}



bool inspect(MaterialCPU* material) {
    bool changed = false;
    ImGui::PathSelect("File", &material->file_path, "resources", FileType_Material);

    changed |= ImGui::ColorEdit4("color_tint", material->color_tint.data, ImGuiColorEditFlags_DisplayHSV);
    changed |= ImGui::ColorEdit4("emissive_tint", material->emissive_tint.data, ImGuiColorEditFlags_DisplayHSV);
    changed |= ImGui::DragFloat("roughness_factor", &material->roughness_factor, 0.01f);
    changed |= ImGui::DragFloat("metallic_factor", &material->metallic_factor, 0.01f);
    changed |= ImGui::DragFloat("normal_factor", &material->normal_factor, 0.01f);

    changed |= ImGui::PathSelect("color_asset_path", &material->color_asset_path, "resources", FileType_Texture);
    changed |= ImGui::PathSelect("orm_asset_path", &material->orm_asset_path, "resources", FileType_Texture);
    changed |= ImGui::PathSelect("normal_asset_path", &material->normal_asset_path, "resources", FileType_Texture);
    changed |= ImGui::PathSelect("emissive_asset_path", &material->emissive_asset_path, "resources", FileType_Texture);

    changed |= ImGui::EnumCombo("cull_mode", &material->cull_mode);

    return changed;
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

