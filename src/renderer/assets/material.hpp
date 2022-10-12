#pragma once

#include <vuk/Pipeline.hpp>
#include <vuk/CommandBuffer.hpp>

#include "string.hpp"

#include "geometry.hpp"
#include "color.hpp"


namespace spellbook {

struct MaterialCPU {
    string name;
    string file_name;

    Color  base_color_tint            = palette::white;
    Color  emissive_tint              = palette::black;
    f32    roughness_factor           = 0.5f;
    f32    metallic_factor            = 0.0f;
    f32    normal_factor              = 0.0f;
    
    string color_asset_path              = "textures/white.sbtex";
    string orm_asset_path                = "textures/white.sbtex";
    string normal_asset_path             = "textures/white.sbtex";
    string emissive_asset_path           = "textures/white.sbtex";

    f32 uv_scale = 1.0f;

    vuk::CullModeFlagBits cull_mode = vuk::CullModeFlagBits::eNone;
};
JSON_IMPL(MaterialCPU, name, base_color_tint, roughness_factor, metallic_factor, normal_factor, emissive_tint, color_asset_path, orm_asset_path, normal_asset_path, emissive_asset_path, uv_scale, cull_mode);

struct MaterialDataGPU {
    v4 base_color_tint;
    v4 emissive_tint;
    v4 roughness_metallic_normal_scale;
};

struct MaterialGPU { // uses master shader
    vuk::PipelineBaseInfo* pipeline;
    vuk::ImageView         color_view;
    vuk::ImageView         orm_view;
    vuk::ImageView         normal_view;
    vuk::ImageView         emissive_view;
    MaterialDataGPU        tints;

    vuk::CullModeFlags cull_mode;
    
    void bind_parameters(vuk::CommandBuffer& cbuf);
    void bind_textures(vuk::CommandBuffer& cbuf);
};

void inspect(MaterialGPU* material);

void save_material(const MaterialCPU&);
MaterialCPU load_material(const string& file_name);

}
