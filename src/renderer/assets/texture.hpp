#pragma once

#include <vuk/Types.hpp>
#include <vuk/Image.hpp>

#include "string.hpp"

#include "geometry.hpp"

namespace spellbook {

struct TextureCPU {
    string name = {};
    string file_name = {};

    v2i         size   = {};
    vuk::Format format = {};
    vector<u8>  pixels = {};
};

JSON_IMPL(TextureCPU, name, size, format);

using TextureGPU = vuk::Texture;

}
