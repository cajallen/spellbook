#pragma once

#include <vuk/Types.hpp>

#include "string.hpp"
#include "json.hpp"
#include "geometry.hpp"

#include "asset_loader.hpp"

namespace spellbook {

struct TextureInfo {
    string original_file;
    int    compression_mode; // CompressionMode
    int    texture_format;   // vuk::Format

    v2i dimensions;

    u64 compressed_byte_size;
    u64 original_byte_size;

    TextureInfo() = default;
    JSON_IMPL(TextureInfo, original_file, compression_mode, texture_format, dimensions, compressed_byte_size, original_byte_size);
};

TextureInfo read_texture_info(AssetFile* file);
void        unpack_texture(TextureInfo* info, const u8* source_buffer, u8* destination);
AssetFile   pack_texture(TextureInfo* info, void* pixel_data);

}

