#pragma once

#include <vuk/Types.hpp>

#include "string.hpp"
#include "json.hpp"
#include "geometry.hpp"

#include "renderer/assets/asset_loader.hpp"

namespace spellbook {

/*
 * Asset files are special file types that contain header data at the start and a binary blob at the end.
 *
 * To load a normally serialized file, we load a file into json and construct the object out of the json
 *
 * To load an asset file, we load a file into an AssetFile, convert it to it's info type and json type,
 * and construct the object out of those two.
 */

struct TextureInfo {
    CompressionMode compression_mode = {};
    u64 compressed_bsize = 0;
    u64 original_bsize = 0;

    TextureInfo() = default;
    JSON_IMPL(TextureInfo, compression_mode, compressed_bsize, original_bsize);
};

TextureInfo read_texture_info(AssetFile* file);
void        unpack_texture(TextureInfo* info, const u8* source_buffer, u8* destination);
AssetFile   pack_texture(TextureInfo* info, void* pixel_data);

}

