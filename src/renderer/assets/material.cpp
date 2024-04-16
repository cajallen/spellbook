#include "material.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/imgui_extra.hpp"
#include "extension/fmt_geometry.hpp"
#include "extension/fmt.hpp"
#include "general/logger.hpp"
#include "general/file/file_path.hpp"
#include "renderer/renderer.hpp"
#include "renderer/gpu_asset_cache.hpp"

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

uint64 make_3d_text_material(uint64 id, vuk::SampledImage& image) {
    if (get_gpu_asset_cache().materials.contains(id))
        return id;

    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/text_3d.vert"_distributed), "shaders/text_3d.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/text_3d.frag"_distributed), "shaders/text_3d.frag"_distributed.abs_string());
        get_renderer().context->create_named_pipeline("text_3d", pci);
    }

    MaterialGPU material_gpu = {};
    material_gpu.pipeline      = get_renderer().context->get_named_pipeline(vuk::Name("text_3d"));
    material_gpu.images.emplace(BASE_COLOR_BINDING, image);
    material_gpu.extra_material_data.push_back(uint8(1));
    material_gpu.cull_mode = vuk::CullModeFlagBits::eNone;
    material_gpu.frame_allocated = false;

    assert_else(material_gpu.pipeline != nullptr);

    get_gpu_asset_cache().materials[id] = std::move(material_gpu);
    return id;
}

uint64 make_ui_material(const FilePath& fp, bool single_channel) {
    uint64 id = hash_path(fp) ^ hash_view("ui_mat");
    if (get_gpu_asset_cache().materials.contains(id))
        return id;

    auto& tex = get_gpu_asset_cache().get_texture_or_upload(fp).value.view;
    vuk::SampledImage image = vuk::make_sampled_image(tex.get(), Sampler().address(Address_Repeat).get());

    return make_ui_material(id, image, single_channel);
}

uint64 make_ui_material(uint64 id, vuk::SampledImage& image, bool single_channel) {
    if (get_gpu_asset_cache().materials.contains(id))
        return id;

    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/ui.vert"_distributed), "shaders/ui.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/ui.frag"_distributed), "shaders/ui.frag"_distributed.abs_string());
        get_renderer().context->create_named_pipeline("ui", pci);
    }

    MaterialGPU material_gpu = {};
    material_gpu.pipeline      = get_renderer().context->get_named_pipeline(vuk::Name("ui"));
    material_gpu.images.emplace(ATLAS_BINDING, image);
    material_gpu.extra_material_data.push_back(uint8(single_channel ? 1 : 0));
    material_gpu.cull_mode = vuk::CullModeFlagBits::eNone;
    material_gpu.frame_allocated = false;

    assert_else(material_gpu.pipeline != nullptr);

    get_gpu_asset_cache().materials[id] = std::move(material_gpu);
    return id;
}

uint64 make_panel_material(const FilePath& fp, const Color& color1, const Color& color2, const v3& vignette_amount) {
    uint64 id = hash_view(fmt_("ui_mat:{},{},{},{}", fp.abs_string(), color1, color2, vignette_amount));
    if (get_gpu_asset_cache().materials.contains(id))
        return id;

    auto& tex = get_gpu_asset_cache().get_texture_or_upload(fp).value.view;
    vuk::SampledImage image = vuk::make_sampled_image(tex.get(), Sampler().address(Address_Repeat).get());

    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/ui.vert"_distributed), "shaders/ui.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/ui_panel.frag"_distributed), "shaders/ui_panel.frag"_distributed.abs_string());
        get_renderer().context->create_named_pipeline("ui_panel", pci);
    }

    struct ExtraMaterialData {
        Color c1, c2;
        v3 vignette;
    } emd;
    emd.c1 = color1;
    emd.c2 = color2;
    emd.vignette = vignette_amount;

    MaterialGPU material_gpu = {};
    material_gpu.pipeline      = get_renderer().context->get_named_pipeline(vuk::Name("ui_panel"));
    material_gpu.images.emplace(ATLAS_BINDING, image);

    material_gpu.extra_material_data.resize(sizeof(ExtraMaterialData));
    memcpy(material_gpu.extra_material_data.data(), &emd, sizeof(ExtraMaterialData));

    material_gpu.cull_mode = vuk::CullModeFlagBits::eNone;
    material_gpu.frame_allocated = false;

    assert_else(material_gpu.pipeline != nullptr);

    get_gpu_asset_cache().materials[id] = std::move(material_gpu);
    return id;
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

    changed |= ImGui::EnumCombo("cull_mode", &material->cull_mode, false);
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

