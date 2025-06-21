#pragma once

#include <vuk/ImageAttachment.hpp>

#include "general/function.hpp"
#include "general/math/geometry.hpp"
#include "general/file/resource.hpp"
#include "general/file/file_path.hpp"

namespace spellbook {

struct TextureExternal {
    static constexpr string_view extension() { return "?"; }
    static constexpr string_view dnd_key() { return "DND_TEXTURE_EXTERNAL"; }
    static FilePath folder() { return get_external_resource_folder(); }
    static function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return vector<string>{".png", ".jpg", ".jpeg"}.contains(path.extension()); }; }
};

struct TextureInfo {
    uint32 pixels_bsize = 0;
};

struct TextureCPU : Resource {
    v2i         size   = {};
    vuk::Format format = {};
    vector<uint8>  pixels = {};
    bool needs_mips = true;

    static constexpr string_view extension() { return ".sbatex"; }
    static constexpr string_view dnd_key() { return "DND_TEXTURE"; }
    static FilePath folder() { return get_resource_folder(); }
    static function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == Resource::extension(); }; }
};

JSON_IMPL(TextureInfo, pixels_bsize);
JSON_IMPL(TextureCPU, size, format);

struct TextureGPU {
    vuk::Allocator* alloc = nullptr;
    vuk::SampledImage value;
    bool frame_allocated;

    TextureGPU() = default;
    TextureGPU(vuk::Allocator& alloc, vuk::SampledImage&& value, bool frame_allocated);
    TextureGPU(TextureGPU&& other) noexcept;
    ~TextureGPU();

    void reset() {
        alloc = nullptr;
    }

    TextureGPU& operator=(TextureGPU&& other) noexcept;

    TextureGPU(const TextureGPU&) = delete;
    TextureGPU& operator=(const TextureGPU&) = delete;
};

template <>
bool     save_resource(const TextureCPU& resource_file);
template <>
TextureCPU& load_resource(const FilePath& input_path, bool assert_exist, bool clear_cache);

FilePath   upload_texture(const TextureCPU& tex_cpu, bool frame_allocation = false);
TextureCPU convert_to_texture(const FilePath& file_name, const FilePath& output_folder, const string& output_name);

}
