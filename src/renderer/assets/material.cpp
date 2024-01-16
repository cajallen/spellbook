#include "material.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "renderer/renderer.hpp"
#include "renderer/gpu_asset_cache.hpp"

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

uint64 upload_material(const MaterialCPU& material_cpu, bool frame_allocation) {
    if (!material_cpu.file_path.is_file())
        return 0;
    
    uint64 material_cpu_hash = hash_path(material_cpu.file_path);

    MaterialGPU material_gpu;
    material_gpu.material_cpu = material_cpu;
    material_gpu.frame_allocated = frame_allocation;
    material_gpu.pipeline      = get_renderer().context->get_named_pipeline(vuk::Name(material_cpu.shader_name));
    assert_else(material_gpu.pipeline != nullptr);
    material_gpu.color = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(material_cpu.color_asset_path).value.view.get(), material_cpu.sampler.get());
    material_gpu.normal = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(material_cpu.normal_asset_path).value.view.get(), material_cpu.sampler.get());
    material_gpu.orm = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(material_cpu.orm_asset_path).value.view.get(), material_cpu.sampler.get());
    material_gpu.emissive = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(material_cpu.emissive_asset_path).value.view.get(), material_cpu.sampler.get());

    material_gpu.tints         = {
        (v4) material_cpu.color_tint,
        (v4) material_cpu.emissive_tint,
        {material_cpu.roughness_factor, material_cpu.metallic_factor, material_cpu.normal_factor, 1.0f},
    };
    material_gpu.cull_mode = material_cpu.cull_mode;
    material_gpu.frame_allocated = frame_allocation;

    get_gpu_asset_cache().materials[material_cpu_hash] = std::move(material_gpu);
    get_gpu_asset_cache().paths[material_cpu_hash] = material_cpu.file_path;
    return material_cpu_hash;
}

void MaterialGPU::update_from_cpu(const MaterialCPU& new_material) {
    pipeline      = get_renderer().context->get_named_pipeline(vuk::Name(new_material.shader_name));
    tints         = {
        (v4) new_material.color_tint,
        (v4) new_material.emissive_tint,
        {new_material.roughness_factor, new_material.metallic_factor, new_material.normal_factor, 1.0f},
    };
    cull_mode = new_material.cull_mode;

    if (material_cpu.color_asset_path != new_material.color_asset_path)
        color = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(new_material.color_asset_path).value.view.get(), new_material.sampler.get());
    if (material_cpu.normal_asset_path != new_material.normal_asset_path)
        normal = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(new_material.normal_asset_path).value.view.get(), new_material.sampler.get());
    if (material_cpu.orm_asset_path != new_material.orm_asset_path)
        orm = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(new_material.orm_asset_path).value.view.get(), new_material.sampler.get());
    if (material_cpu.emissive_asset_path != new_material.emissive_asset_path)
        emissive = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(new_material.emissive_asset_path).value.view.get(), new_material.sampler.get());

    material_cpu = new_material;
}



bool inspect(MaterialCPU* material) {
    bool changed = false;
    ImGui::PathSelect<MaterialCPU>("File", &material->file_path);

    changed |= inspect_dependencies(material->dependencies, material->file_path);
    
    changed |= ImGui::ColorEdit4("color_tint", material->color_tint.data, ImGuiColorEditFlags_DisplayHSV);
    changed |= ImGui::ColorEdit4("emissive_tint", material->emissive_tint.data, ImGuiColorEditFlags_DisplayHSV);
    changed |= ImGui::DragFloat("roughness_factor", &material->roughness_factor, 0.01f);
    changed |= ImGui::DragFloat("metallic_factor", &material->metallic_factor, 0.01f);
    changed |= ImGui::DragFloat("normal_factor", &material->normal_factor, 0.01f);

    changed |= ImGui::PathSelect<TextureCPU>("color_asset_path", &material->color_asset_path);
    changed |= ImGui::PathSelect<TextureCPU>("orm_asset_path", &material->orm_asset_path);
    changed |= ImGui::PathSelect<TextureCPU>("normal_asset_path", &material->normal_asset_path);
    changed |= ImGui::PathSelect<TextureCPU>("emissive_asset_path", &material->emissive_asset_path);

    changed |= ImGui::EnumCombo("cull_mode", &material->cull_mode);
    ImGui::InputText("shader", &material->shader_name);
    ImGui::SameLine();
    changed |= ImGui::Button(ICON_FA_REFRESH);

    return changed;
}

void inspect(MaterialGPU* material) {
    ImGui::Text("Base Color");
    ImGui::Image(&*get_renderer().imgui_images.emplace(material->color), {100, 100});
    ImGui::ColorEdit4("Tint##BaseColor", material->tints.color_tint.data);
    
    ImGui::BeginGroup();
    ImGui::Text("ORM");
    ImGui::Image(&*get_renderer().imgui_images.emplace(material->orm), {100, 100});
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Normals");
    ImGui::Image(&*get_renderer().imgui_images.emplace(material->normal), {100, 100});
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Emissive");
    ImGui::Image(&*get_renderer().imgui_images.emplace(material->emissive), {100, 100});
    ImGui::EndGroup();
    ImGui::ColorEdit4("Emissive Tint", material->tints.emissive_tint.data);

    ImGui::DragFloat("Roughness Factor", &material->tints.roughness_metallic_normal_scale.x, 0.01f);
    ImGui::DragFloat("Metallic Factor", &material->tints.roughness_metallic_normal_scale.y, 0.01f);
    ImGui::DragFloat("Normal Factor", &material->tints.roughness_metallic_normal_scale.z, 0.01f);
    ImGui::DragFloat("UV Scale", &material->tints.roughness_metallic_normal_scale.w, 0.01f);
}

void save_material(MaterialCPU& material_cpu) {
    auto j = from_jv<json>(to_jv(material_cpu));

    assert_else(material_cpu.file_path.extension() == MaterialCPU::extension());
    
    file_dump(j, material_cpu.file_path.abs_string());
}

MaterialCPU load_material(const FilePath& file_path) {
    assert_else(file_path.extension() == MaterialCPU::extension());
    
    json j = parse_file(file_path.abs_string());
    auto material_cpu = from_jv<MaterialCPU>(to_jv(j));
    material_cpu.file_path = file_path;

    return material_cpu;
}


}

