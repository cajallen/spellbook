#pragma once

#include <vuk/Types.hpp>
#include <vuk/Image.hpp>

#include "lib/string.hpp"
#include "lib/geometry.hpp"

namespace spellbook {

struct TextureCPU {
    string file_path = "";

    v2i         size   = {};
    vuk::Format format = {};
    vector<u8>  pixels = {};
};

JSON_IMPL(TextureCPU, size, format);

using TextureGPU = vuk::Texture;

}
