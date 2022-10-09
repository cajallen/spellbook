#include "texture.hpp"

#include "console.hpp"

#include "renderer/assets/asset_loader.hpp"
#include "renderer/assets/texture_asset.hpp"

namespace spellbook {

TextureCPU load_texture(const string_view file_name) {
    AssetFile file;
    bool              loaded = load_binary_file(file_name, &file);
    if (!loaded) {
        console_error("Error loading texture", "asset", ErrorType_Severe);
        return {};
    }

    TextureInfo tex_info = read_texture_info(&file);

    void* data = malloc(tex_info.original_byte_size);
    unpack_texture(&tex_info, (u8*) file.binary_blob.data(), (u8*) data);

    TextureCPU texture;
    texture.data = (u8*) data;
    texture.size = tex_info.dimensions;
    
    return texture;
}

void save_texture(const TextureCPU&) {
    assert_else(false && "NYI");
}


}