#include "texture_asset.hpp"

#include <filesystem>
#include <iostream>
#include <lz4.h>
#include <stb_image.h>

#include "game.hpp"
#include "console.hpp"
#include "file.hpp"

#include "lib_ext/fmt_geometry.hpp"

namespace fs = std::filesystem;

namespace spellbook {

TextureCPU load_texture(const string& file_name) {
    // TODO: CompressionMode
    AssetFile asset_file = load_asset_file(file_name);

    constexpr array expected_type = {'T', 'E', 'X'};
    assert_else(asset_file.version == 2 && asset_file.type == expected_type)
        return {};

    TextureInfo texture_info = from_jv<TextureInfo>(*asset_file.asset_json["texture_info"]);
    TextureCPU  texture_cpu  = from_jv<TextureCPU>(*asset_file.asset_json["texture_cpu"]);
    texture_cpu.file_path = file_name;
    
    texture_cpu.pixels.resize(texture_info.pixels_bsize);
    LZ4_decompress_safe((const char*) asset_file.binary_blob.data(),
        (char*) texture_cpu.pixels.data(),
        asset_file.binary_blob.size(),
        (s32) texture_cpu.pixels.size());
    
    return texture_cpu;
}

void save_texture(TextureCPU& texture_cpu) {
    AssetFile file;
    file.file_name = texture_cpu.file_path;
    file.type      = {'T', 'E', 'X'};
    file.version   = 2;

    TextureInfo texture_info;
    texture_info.pixels_bsize = texture_cpu.pixels.size();

    s32 compress_staging = LZ4_compressBound(s32(texture_info.pixels_bsize));
    file.binary_blob.resize(compress_staging);
    s32 compressed_bsize = LZ4_compress_default((char*) texture_cpu.pixels.data(),
        (char*) file.binary_blob.data(),
        s32(texture_cpu.pixels.size()),
        s32(compress_staging));
    file.binary_blob.resize(compressed_bsize);
    texture_info.compression_mode = CompressionMode_Lz4;

    json j;
    j["texture_cpu"]  = make_shared<json_value>(to_jv(texture_cpu));
    j["texture_info"] = make_shared<json_value>(to_jv(texture_info));
    file.asset_json   = j;

    save_asset_file(file);
}

TextureCPU convert_to_texture(const string& file_name, const string& output_folder, const string& output_name) {
    fs::create_directory(game.resource_folder);
    fs::create_directory(fs::path(game.resource_folder) / output_folder);

    const fs::path file_path = fs::path(file_name);

    const auto& ext = file_path.extension().string();

    TextureCPU texture;
    fs::path   out_path = fs::path(output_folder) / fs::path(output_name + extension(FileType_Texture));
    texture.file_path   = out_path.string();
    int channels;
    if (ext == ".hdr") {
        assert_else(false && "NYI");
        f32* pixel_data = stbi_loadf(file_name.c_str(), &texture.size.x, &texture.size.y, &channels, STBI_rgb_alpha);
        assert_else(pixel_data) {
            free(pixel_data);
            return {};
        }
        texture.format = vuk::Format::eR32G32B32A32Sfloat;
    } else {
        u8* pixel_data = stbi_load(file_name.c_str(), &texture.size.x, &texture.size.y, &channels, STBI_rgb_alpha);
        assert_else(pixel_data) {
            free(pixel_data);
            return {};
        }
        texture.pixels.resize(texture.size.x * texture.size.y * 4);
        memcpy(texture.pixels.data(), pixel_data, texture.pixels.size());
        texture.format = vuk::Format::eR8G8B8A8Srgb;
        free(pixel_data);
    }
    return texture;
}


}
