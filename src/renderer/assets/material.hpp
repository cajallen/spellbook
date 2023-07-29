#pragma once

#include <vuk/CommandBuffer.hpp>
#include <vuk/SampledImage.hpp>

#include "general/string.hpp"
#include "general/math/geometry.hpp"
#include "general/color.hpp"
#include "renderer/samplers.hpp"
#include "general/file_path.hpp"

namespace vuk {
struct PipelineBaseInfo;
}

namespace spellbook {

// This should really be MaterialPrefab, and we should convert the paths into ids for a MaterialCPU
struct MaterialCPU {
    FilePath file_path;
    vector<FilePath> dependencies;

    Color color_tint       = palette::white;
    Color emissive_tint    = palette::black;
    float   roughness_factor = 0.5f;
    float   metallic_factor  = 0.0f;
    float   normal_factor    = 0.0f;

    FilePath color_asset_path    = FilePath("white", true);
    FilePath orm_asset_path      = FilePath("white", true);
    FilePath normal_asset_path   = FilePath("white", true);
    FilePath emissive_asset_path = FilePath("white", true);

    Sampler sampler = Sampler().address(Address_Mirrored).anisotropy(true);

    vuk::CullModeFlagBits cull_mode = vuk::CullModeFlagBits::eNone;

    string shader_name = "textured_model";
};

JSON_IMPL(MaterialCPU, color_tint, roughness_factor, metallic_factor, normal_factor, emissive_tint, color_asset_path,
        orm_asset_path, normal_asset_path, emissive_asset_path, sampler, cull_mode, shader_name);

struct MaterialDataGPU {
    v4 color_tint;
    v4 emissive_tint;
    v4 roughness_metallic_normal_scale;
};

struct MaterialGPU {
    MaterialCPU material_cpu;
    // uses master shader
    vuk::PipelineBaseInfo* pipeline;
    vuk::SampledImage      color    = vuk::SampledImage(vuk::SampledImage::Global{});
    vuk::SampledImage      orm      = vuk::SampledImage(vuk::SampledImage::Global{});
    vuk::SampledImage      normal   = vuk::SampledImage(vuk::SampledImage::Global{});
    vuk::SampledImage      emissive = vuk::SampledImage(vuk::SampledImage::Global{});
    MaterialDataGPU        tints;

    vuk::CullModeFlags cull_mode;

    bool frame_allocated = false;
    
    void bind_parameters(vuk::CommandBuffer& cbuf);
    void bind_textures(vuk::CommandBuffer& cbuf);

    void update_from_cpu(const MaterialCPU& new_material);
};

bool inspect(MaterialCPU* material);
void inspect(MaterialGPU* material);

uint64         upload_material(const MaterialCPU&, bool frame_allocation = false);
void        save_material(MaterialCPU&);
MaterialCPU load_material(const FilePath& file_name);

}
