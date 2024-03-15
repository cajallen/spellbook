#include "material.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "renderer/renderer.hpp"
#include "renderer/gpu_asset_cache.hpp"

#include "extension/fmt.hpp"
#include "general/file/file_path.hpp"

namespace spellbook {

void MaterialGPU::bind_parameters(vuk::CommandBuffer& cbuf) {
    void* data = cbuf._map_scratch_buffer(0, MATERIAL_BINDING, extra_material_data.size());
    memcpy(data, extra_material_data.data(), extra_material_data.size());
};

void MaterialGPU::bind_textures(vuk::CommandBuffer& cbuf) {
    for (const auto& [binding, image] : images) {
        cbuf.bind_image(0, binding, image.global.iv).bind_sampler(0, binding, image.global.sci);
    }
};

void make_ui_material(uint64 id, vuk::SampledImage& image) {
    MaterialGPU material_gpu = {};
    material_gpu.frame_allocated = false;
    material_gpu.pipeline      = get_renderer().context->get_named_pipeline(vuk::Name("ui"));
    material_gpu.images.emplace(ATLAS_BINDING, image);
    material_gpu.cull_mode = vuk::CullModeFlagBits::eNone;
    material_gpu.frame_allocated = false;

    assert_else(material_gpu.pipeline != nullptr);

    get_gpu_asset_cache().materials[id] = std::move(material_gpu);
}

uint64 upload_material(const MaterialCPU& material_cpu, bool frame_allocation) {
    if (!material_cpu.file_path.is_file())
        return 0;
    
    uint64 material_cpu_hash = hash_path(material_cpu.file_path);

    MaterialGPU material_gpu;
    material_gpu.frame_allocated = frame_allocation;
    material_gpu.pipeline      = get_renderer().context->get_named_pipeline(vuk::Name(material_cpu.shader_name));
    assert_else(material_gpu.pipeline != nullptr);

    material_gpu.images.emplace(BASE_COLOR_BINDING, vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(material_cpu.color_asset_path).value.view.get(), material_cpu.sampler.get()));
    material_gpu.images.emplace(EMISSIVE_BINDING, vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(material_cpu.emissive_asset_path).value.view.get(), material_cpu.sampler.get()));
    material_gpu.images.emplace(NORMAL_BINDING, vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(material_cpu.normal_asset_path).value.view.get(), material_cpu.sampler.get()));
    material_gpu.images.emplace(ORM_BINDING, vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(material_cpu.orm_asset_path).value.view.get(), material_cpu.sampler.get()));

    BasicMaterialDataGPU gpu_tints = {
        (v4) material_cpu.color_tint,
        (v4) material_cpu.emissive_tint,
        {material_cpu.roughness_factor, material_cpu.metallic_factor, material_cpu.normal_factor, 1.0f},
    };
    material_gpu.extra_material_data.resize(sizeof(BasicMaterialDataGPU));
    memcpy(material_gpu.extra_material_data.data(), &gpu_tints, sizeof(BasicMaterialDataGPU));
    material_gpu.cull_mode = material_cpu.cull_mode;
    material_gpu.frame_allocated = frame_allocation;

    get_gpu_asset_cache().materials[material_cpu_hash] = std::move(material_gpu);
    get_gpu_asset_cache().paths[material_cpu_hash] = material_cpu.file_path;
    return material_cpu_hash;
}

void MaterialGPU::update_from_cpu(const MaterialCPU& new_material) {
    pipeline      = get_renderer().context->get_named_pipeline(vuk::Name(new_material.shader_name));
    BasicMaterialDataGPU tints = {
        (v4) new_material.color_tint,
        (v4) new_material.emissive_tint,
        {new_material.roughness_factor, new_material.metallic_factor, new_material.normal_factor, 1.0f},
    };
    cull_mode = new_material.cull_mode;

    images.at(BASE_COLOR_BINDING) = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(new_material.color_asset_path).value.view.get(), new_material.sampler.get());
    images.at(NORMAL_BINDING) = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(new_material.normal_asset_path).value.view.get(), new_material.sampler.get());
    images.at(ORM_BINDING) = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(new_material.orm_asset_path).value.view.get(), new_material.sampler.get());
    images.at(EMISSIVE_BINDING) = vuk::make_sampled_image(get_gpu_asset_cache().get_texture_or_upload(new_material.emissive_asset_path).value.view.get(), new_material.sampler.get());
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
    ImGui::Text("TODO");
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

