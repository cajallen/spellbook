#pragma once

#include <vuk/Types.hpp>
#include <vuk/Image.hpp>

#include "general/string.hpp"
#include "general/math/geometry.hpp"
#include "general/file_path.hpp"

namespace spellbook {

struct TextureCPU {
    FilePath file_path;
    vector<FilePath> dependencies;

    v2i         size   = {};
    vuk::Format format = {};
    vector<uint8>  pixels = {};
};

JSON_IMPL(TextureCPU, size, format);

struct TextureGPU {
    vuk::Texture value;
    bool frame_allocated;
};

FilePath upload_texture(const TextureCPU& tex_cpu, bool frame_allocation = false);

}
