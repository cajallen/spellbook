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

    TextureCPU() = default;
    TextureCPU(string name, string file_name, v2i size, vuk::Format format, vector<u8> pixels) : name(name), file_name(file_name), size(size), format(format), pixels(pixels) {}
    JSON_IMPL(TextureCPU, name, size, format);

    // Skips name/file_name, only references the actual mesh
    u64 contents_hash() const;
};

using TextureGPU = vuk::Texture;

}
