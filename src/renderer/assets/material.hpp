#pragma once

#include <vuk/Pipeline.hpp>
#include <vuk/CommandBuffer.hpp>

#include "string.hpp"

#include "geometry.hpp"
#include "color.hpp"

#include "renderer/render_scene.hpp"


namespace spellbook {

struct MaterialCPU {
    string name;
    string file_name;

    Color  base_color_tint            = palette::white;
    Color  emissive_tint              = palette::black;
    f32    roughness_factor           = 0.5f;
    f32    metallic_factor            = 0.0f;
    f32    normal_factor              = 0.0f;
    
    string base_color_texture         = "textures/white.tx";
    string orm_texture                = "textures/white.tx";
    string normal_texture             = "textures/white.tx";
    string emissive_texture           = "textures/white.tx";

    f32 uv_scale = 1.0f;

    vuk::CullModeFlags cull_mode = vuk::CullModeFlagBits::eNone;

    MaterialCPU() = default;
    JSON_IMPL(MaterialCPU, name, base_color_tint, base_color_texture, roughness_factor, metallic_factor, metallic_roughness_texture, normal_factor, normal_texture, emissive_tint, emissive_texture, uv_scale, cull_mode)
};

struct MaterialDataGPU {
    v4 base_color_tint;
    v4 emissive_tint;
    v4 roughness_metallic_normal_scale;
};

struct MaterialGPU { // uses master shader
    vuk::PipelineBaseInfo* pipeline;
    vuk::ImageView         base_color_view;
    vuk::ImageView         metallic_roughness_view;
    vuk::ImageView         normal_view;
    vuk::ImageView         emissive_view;
    MaterialDataGPU        tints;

    vuk::CullModeFlags cull_mode;

    MaterialGPU()                         = default;
    MaterialGPU(const MaterialGPU& other) = default;
    MaterialGPU(MaterialGPU&& other)      = default;
    virtual void bind_parameters(vuk::CommandBuffer& cbuf);
    virtual void bind_textures(vuk::CommandBuffer& cbuf);
};

void inspect(MaterialGPU* material);

void save_material(const MaterialCPU&);
MaterialCPU load_material(const string_view file_name);

}
