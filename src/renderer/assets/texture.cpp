#include "texture.hpp"

#include <vuk/Partials.hpp>
#include <lz4/lz4.h>
#include <stb_image.h>

#include "general/logger.hpp"
#include "general/file/file_cache.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "renderer/renderer.hpp"

namespace spellbook {

FilePath upload_texture(const TextureCPU& tex_cpu, bool frame_allocation) {
    assert_else(tex_cpu.file_path.is_file());
    uint64 tex_cpu_hash = hash_path(tex_cpu.file_path);
    vuk::Allocator& alloc = frame_allocation ? *get_renderer().frame_allocator : *get_renderer().global_allocator;
    auto [tex, tex_fut] = vuk::create_texture(alloc, tex_cpu.format, vuk::Extent3D(tex_cpu.size), (void*) tex_cpu.pixels.data(), tex_cpu.needs_mips);
    get_renderer().context->set_name(tex, vuk::Name(tex_cpu.file_path.rel_string()));
    get_renderer().enqueue_setup(std::move(tex_fut));

    get_gpu_asset_cache().textures[tex_cpu_hash] = {std::move(tex), frame_allocation};
    assert_else(get_gpu_asset_cache().textures[tex_cpu_hash].value.image->image != VK_NULL_HANDLE);

    return tex_cpu.file_path;
}

TextureCPU load_texture(const FilePath& file_path) {
    AssetFile& asset_file = get_file_cache().load_asset(file_path);

    TextureInfo texture_info = from_jv<TextureInfo>(*asset_file.asset_json["texture_info"]);
    TextureCPU  texture_cpu  = from_jv<TextureCPU>(*asset_file.asset_json["texture_cpu"]);
    texture_cpu.file_path = file_path;

    texture_cpu.pixels.resize(texture_info.pixels_bsize);
    LZ4_decompress_safe((const char*) asset_file.binary_blob.data(),
        (char*) texture_cpu.pixels.data(),
        asset_file.binary_blob.size(),
        (int32) texture_cpu.pixels.size());

    return texture_cpu;
}

void save_texture(const TextureCPU& texture_cpu) {
    AssetFile file;
    file.file_path = texture_cpu.file_path;
    file.version   = 2;

    TextureInfo texture_info;
    texture_info.pixels_bsize = texture_cpu.pixels.size();

    int32 compress_staging = LZ4_compressBound(int32(texture_info.pixels_bsize));
    file.binary_blob.resize(compress_staging);
    int32 compressed_bsize = LZ4_compress_default((char*) texture_cpu.pixels.data(),
        (char*) file.binary_blob.data(),
        int32(texture_cpu.pixels.size()),
        int32(compress_staging));
    file.binary_blob.resize(compressed_bsize);

    json j;
    j["texture_cpu"]  = make_shared<json_value>(to_jv(texture_cpu));
    j["texture_info"] = make_shared<json_value>(to_jv(texture_info));
    file.asset_json   = j;

    get_file_cache().parsed_assets[file.file_path] = file;
    save_asset_file(file);
}

TextureCPU convert_to_texture(const FilePath& input_file_path, const FilePath& output_folder, const string& output_name) {
    fs::create_directories(output_folder.abs_path());

    TextureCPU texture;
    texture.file_path = FilePath(output_folder.rel_string() + output_name + string(TextureCPU::extension()));
    int channels;
    if (texture.file_path.extension() == ".hdr") {
        log_error(".hdr NYI");
        float* pixel_data = stbi_loadf(input_file_path.abs_string().c_str(), &texture.size.x, &texture.size.y, &channels, STBI_rgb_alpha);
        assert_else(pixel_data) {
            free(pixel_data);
            return {};
        }
        texture.format = vuk::Format::eR32G32B32A32Sfloat;
    } else {
        uint8* pixel_data = stbi_load(input_file_path.abs_string().c_str(), &texture.size.x, &texture.size.y, &channels, STBI_rgb_alpha);
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
