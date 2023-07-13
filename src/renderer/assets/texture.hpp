#pragma once

#include <vuk/Types.hpp>
#include <vuk/Image.hpp>

#include "general/string.hpp"
#include "general/math/geometry.hpp"

namespace spellbook {

struct TextureCPU {
    string file_path;
    vector<string> dependencies;

    v2i         size   = {};
    vuk::Format format = {};
    vector<uint8>  pixels = {};
};

JSON_IMPL(TextureCPU, size, format);

struct TextureGPU {
    vuk::Texture value;
    bool frame_allocated;
};

string upload_texture(const TextureCPU& tex_cpu, bool frame_allocation = false);

}
