#pragma once

#include <vuk/CommandBuffer.hpp>
#include <vuk/SampledImage.hpp>

#include "general/string.hpp"
#include "general/geometry.hpp"
#include "general/color.hpp"
#include "renderer/samplers.hpp"


namespace vuk {
struct PipelineBaseInfo;
}

namespace spellbook {

struct MaterialCPU {
    string file_path;

    Color color_tint       = palette::white;
    Color emissive_tint    = palette::black;
    f32   roughness_factor = 0.5f;
    f32   metallic_factor  = 0.0f;
    f32   normal_factor    = 0.0f;
    v2   emissive_dot_smoothstep    = v2(-2.0f, -1.0f);

    string color_asset_path    = "textures/white.sbtex";
    string orm_asset_path      = "textures/white.sbtex";
    string normal_asset_path   = "textures/white.sbtex";
    string emissive_asset_path = "textures/white.sbtex";

    Sampler sampler = Sampler().anisotropy(true);

    vuk::CullModeFlagBits cull_mode = vuk::CullModeFlagBits::eNone;

    string shader_name = "textured_model";
};

JSON_IMPL(MaterialCPU, color_tint, roughness_factor, metallic_factor, normal_factor, emissive_dot_smoothstep, emissive_tint, color_asset_path,
        orm_asset_path, normal_asset_path, emissive_asset_path, sampler, cull_mode);

struct MaterialDataGPU {
    v4 color_tint;
    v4 emissive_tint;
    v4 roughness_metallic_normal_scale;
    v4 emissive_dot_smoothstep;
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

string upload_material(const MaterialCPU&, bool frame_allocation = false);
void        save_material(const MaterialCPU&);
MaterialCPU load_material(const string& file_name);

}
