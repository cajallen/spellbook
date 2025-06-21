#include "texture.hpp"

#include <vuk/vsl/Core.hpp>
#include <lz4/lz4.h>
#include <stb_image.h>

#include "general/logger.hpp"
#include "general/file/file_cache.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "renderer/renderer.hpp"
#include "renderer/runtime.hpp"

namespace spellbook {

template<>
TextureCPU& load_resource(const FilePath& input_path, bool assert_exists, bool clear_cache) {
    if (clear_cache && cpu_resource_cache<TextureCPU>().contains(input_path))
        cpu_resource_cache<TextureCPU>().erase(input_path);
    if (cpu_resource_cache<TextureCPU>().contains(input_path))
        return *cpu_resource_cache<TextureCPU>()[input_path];

    TextureCPU& texture = *cpu_resource_cache<TextureCPU>().emplace(input_path, std::make_unique<TextureCPU>()).first->second;

    bool exists = input_path.exists();
    bool corrext = input_path.extension() == TextureCPU::extension();
    if (assert_exists) {
        assert_else(exists && corrext)
            return texture;
    } else {
        check_else(exists && corrext)
            return texture;
    }

    AssetFile& asset_file = get_file_cache().load_asset(input_path);

    TextureInfo texture_info = from_jv<TextureInfo>(*asset_file.asset_json["texture_info"]);
    texture = from_jv<TextureCPU>(*asset_file.asset_json["texture_cpu"]);
    texture.file_path = input_path;

    texture.pixels.resize(texture_info.pixels_bsize);
    LZ4_decompress_safe((const char*) asset_file.binary_blob.data(),
        (char*) texture.pixels.data(),
        asset_file.binary_blob.size(),
        (int32) texture.pixels.size());

    return texture;
}

template<>
bool save_resource(const TextureCPU& texture_cpu) {
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
    return true;
}

TextureCPU convert_to_texture(const FilePath& input_file_path, const FilePath& output_folder, const string& output_name) {
    create_directories(output_folder);

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

FilePath upload_texture(const TextureCPU& tex_cpu, bool frame_allocation) {
    assert_else(tex_cpu.file_path.is_file());
    uint64 tex_cpu_hash = hash_path(tex_cpu.file_path);
    vuk::Allocator& alloc = frame_allocation ? *get_renderer().frame_allocator : *get_renderer().global_allocator;

    // TODO: mips
    auto ia = vuk::ImageAttachment::from_preset(vuk::ImageAttachment::Preset::eMap2D, tex_cpu.format, vuk::Extent3D(tex_cpu.size), vuk::Samples::e1);
    auto [_image, _view, _future] = vuk::create_image_and_view_with_data(alloc, vuk::DomainFlagBits::eTransferOnTransfer, ia, tex_cpu.pixels.data());
    
    // Our texture object will handle lifetimes of image and view
    _image.release();
    _view.release();

    get_renderer().runtime->runtime->set_name(ia.image.image, vuk::Name(tex_cpu.file_path.rel_string()));
    get_renderer().runtime->runtime->set_name(ia.image_view.payload, vuk::Name(tex_cpu.file_path.rel_string()));
    _future.set_name(tex_cpu.file_path.rel_string());

    get_renderer().enqueue_setup(std::move(_future));

    get_gpu_asset_cache().textures[tex_cpu_hash] = TextureGPU(alloc, vuk::SampledImage{ia, Sampler().get()}, frame_allocation ? get_renderer().frame_timer.current_frame : ~0u);
    assert_else(get_gpu_asset_cache().textures[tex_cpu_hash].value.ia.image.image != VK_NULL_HANDLE);

    return tex_cpu.file_path;
}

TextureGPU::TextureGPU(vuk::Allocator& alloc, vuk::SampledImage&& value, bool frame_allocated) 
    : alloc(&alloc)
    , value(value)
    , frame_allocated(frame_allocated) {}

TextureGPU::~TextureGPU() {
    if (alloc) {
        vuk::deallocate(*alloc, value.ia.image);
        vuk::deallocate(*alloc, value.ia.image_view);
    }
}

TextureGPU::TextureGPU(TextureGPU&& other) noexcept
    : alloc(other.alloc)
    , value(std::move(other.value))
    , frame_allocated(other.frame_allocated) {
    other.reset();
}

TextureGPU& TextureGPU::operator=(TextureGPU&& other) noexcept {
    alloc = other.alloc;
    value = std::move(other.value);
    frame_allocated = other.frame_allocated;
    other.reset();
    return *this;
}

}
