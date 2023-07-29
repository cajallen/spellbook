#pragma once

#include <vuk/Types.hpp>

#include "general/json.hpp"
#include "renderer/assets/texture.hpp"
#include "renderer/assets/asset_loader.hpp"
#include "general/file_path.hpp"

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
    uint32          pixels_bsize   = 0;
};

JSON_IMPL(TextureInfo, compression_mode, pixels_bsize);

TextureCPU convert_to_texture(const FilePath& file_name, const FilePath& output_folder, const string& output_name);
TextureCPU load_texture(const FilePath& file_name);
void       save_texture(const TextureCPU& texture_cpu);

}
