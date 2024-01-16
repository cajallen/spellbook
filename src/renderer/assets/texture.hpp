#pragma once

#include <vuk/Types.hpp>
#include <vuk/Image.hpp>

#include "general/string.hpp"
#include "general/math/geometry.hpp"
#include "general/file/file_path.hpp"
#include "general/file/resource.hpp"

namespace spellbook {

struct TextureExternal {
    static constexpr string_view extension() { return "?"; }
    static constexpr string_view dnd_key() { return "DND_TEXTURE_EXTERNAL"; }
    static FilePath folder() { return get_external_resource_folder(); }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return vector<string>{".png", ".jpg", ".jpeg"}.contains(path.extension()); }; }
};

struct TextureInfo {
    uint32 pixels_bsize = 0;
};

struct TextureCPU : Resource {
    v2i         size   = {};
    vuk::Format format = {};
    vector<uint8>  pixels = {};

    static constexpr string_view extension() { return ".sbatex"; }
    static constexpr string_view dnd_key() { return "DND_TEXTURE"; }
    static FilePath folder() { return get_resource_folder(); }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == Resource::extension(); }; }
};

JSON_IMPL(TextureInfo, pixels_bsize);
JSON_IMPL(TextureCPU, size, format);

struct TextureGPU {
    vuk::Texture value;
    bool frame_allocated;
};

TextureCPU load_texture(const FilePath& file_name);
void       save_texture(const TextureCPU& texture_cpu);
FilePath   upload_texture(const TextureCPU& tex_cpu, bool frame_allocation = false);
TextureCPU convert_to_texture(const FilePath& file_name, const FilePath& output_folder, const string& output_name);

}
