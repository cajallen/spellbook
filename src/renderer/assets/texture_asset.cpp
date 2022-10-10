#include "texture_asset.hpp"

#include <lz4.h>
#include <iostream>

#include "console.hpp"

#include "lib_ext/fmt_geometry.hpp"

namespace spellbook {

TextureCPU load_texture(const string& file_name) {
    // TODO: CompressionMode
    AssetFile asset_file = load_asset_file(file_name);

    constexpr array expected_type = {'T','E','X'};
    assert_else(asset_file.version == 2 && asset_file.type == expected_type)
        return {};
    
    TextureInfo texture_info = TextureInfo(*asset_file.asset_json["texture_info"]);
    TextureCPU texture_cpu = TextureCPU(*asset_file.asset_json["texture_cpu"]);

    vector<u8> decompressed(texture_info.pixels_bsize);
    LZ4_decompress_safe((const char*) asset_file.binary_blob.data(), (char*) decompressed.data(), asset_file.binary_blob.size(), (s32) decompressed.size());

    memcpy(texture_cpu.pixels.data(), decompressed.data(), texture_info.pixels_bsize);

    return texture_cpu;
}

void save_texture(TextureCPU& texture_cpu) {
    AssetFile file;
    file.file_name = texture_cpu.file_name;
    file.type = {'T','E','X'};
    file.version = 2;

    TextureInfo texture_info;
    texture_info.pixels_bsize = texture_cpu.pixels.size() * sizeof(u32);
    
    s32 compress_staging = LZ4_compressBound(s32(texture_info.pixels_bsize));
    file.binary_blob.resize(compress_staging);
    s32 compressed_bsize = LZ4_compress_default((char*) texture_cpu.pixels.data(), (char*) file.binary_blob.data(), s32(texture_cpu.pixels.size()), s32(compress_staging));
    file.binary_blob.resize(compressed_bsize);
    texture_info.compression_mode = CompressionMode_Lz4;
    
    json j;
    j["mesh_cpu"] = make_shared<json_value>(texture_cpu);
    j["texture_info"] = make_shared<json_value>(texture_info);
    file.asset_json = j;

    save_asset_file(file);
}

}
