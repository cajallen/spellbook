#pragma once

#include <vuk/CommandBuffer.hpp>
#include <vuk/SampledImage.hpp>

#include "general/string.hpp"
#include "general/color.hpp"
#include "general/math/geometry.hpp"
#include "general/file/file_path.hpp"
#include "general/file/resource.hpp"
#include "renderer/samplers.hpp"

namespace vuk {
struct PipelineBaseInfo;
}

namespace spellbook {

// This should really be MaterialPrefab, and we should convert the paths into ids for a MaterialCPU
struct MaterialCPU : Resource {
    Color color_tint       = palette::white;
    Color emissive_tint    = palette::black;
    float   roughness_factor = 0.5f;
    float   metallic_factor  = 0.0f;
    float   normal_factor    = 0.0f;

    FilePath color_asset_path    = "white"_symbolic;
    FilePath orm_asset_path      = "white"_symbolic;
    FilePath normal_asset_path   = "white"_symbolic;
    FilePath emissive_asset_path = "white"_symbolic;

    Sampler sampler = Sampler().address(Address_Repeat).anisotropy(true);

    vuk::CullModeFlagBits cull_mode = vuk::CullModeFlagBits::eNone;

    string shader_name = "textured_model";

    static constexpr string_view extension() { return ".sbjmat"; }
    static constexpr string_view dnd_key() { return "DND_MATERIAL"; }
    static FilePath folder() { return get_resource_folder(); }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == MaterialCPU::extension(); }; }
};

JSON_IMPL(MaterialCPU, color_tint, roughness_factor, metallic_factor, normal_factor, emissive_tint, color_asset_path,
        orm_asset_path, normal_asset_path, emissive_asset_path, sampler, cull_mode, shader_name);

struct BasicMaterialDataGPU {
    v4 color_tint;
    v4 emissive_tint;
    v4 roughness_metallic_normal_scale;
};

struct MaterialGPU {
    // uses master shader
    vuk::PipelineBaseInfo* pipeline;

    umap<uint32, vuk::SampledImage> images;
    vector<uint8> extra_material_data;

    vuk::CullModeFlags cull_mode;

    bool frame_allocated = false;

    void bind_parameters(vuk::CommandBuffer& cbuf);
    void bind_textures(vuk::CommandBuffer& cbuf);

    void update_from_cpu(const MaterialCPU& new_material);
};

uint64 make_ui_material(const FilePath& texture);
uint64 make_ui_material(uint64 id, vuk::SampledImage& image);

bool inspect(MaterialCPU* material);
void inspect(MaterialGPU* material);

uint64      upload_material(const MaterialCPU&, bool frame_allocation = false);
void        save_material(MaterialCPU&);
MaterialCPU load_material(const FilePath& file_name);

}
