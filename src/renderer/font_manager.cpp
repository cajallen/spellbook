#include "font_manager.hpp"

#include <ft2build.h>
#include <freetype/freetype.h>

#include <stb_rect_pack.h>

#include "extension/fmt.hpp"
#include "general/logger.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "renderer/renderer.hpp"
#include "renderer/render_scene.hpp"

namespace spellbook {

void FontManager::setup() {
    if (initialized)
        return;
    initialized = true;
    auto error = FT_Init_FreeType(&library);
    if (error) {
        log_warning("Font manager initialization failure");
        return;
    }
}


Font* FontManager::load_font(const FilePath& path, uint32 font_size, bool ui) {
    FontRequestInfo request = {path, font_size};

    FT_Face face;
    string path_abs_string = path.abs_string();
    auto error = FT_New_Face(library, path_abs_string.c_str(), 0, &face);
    if (error) {
        log_warning("Font loading error");
        return nullptr;
    }

    fonts[request] = Font{};
    Font& font = fonts.at(request);
    font.size = font_size;

    error = FT_Set_Pixel_Sizes(face, 0, font_size);

    if (error) {
        log_warning("Font size setting error");
        FT_Done_Face(face);
        return nullptr;
    }

    TextureCPU texture;
    texture.file_path = FilePath(fmt_("{}_font_size:{}", path.abs_string(), font_size), FilePathLocation_Symbolic);
    font.id = hash_path(texture.file_path);

    vector<vector<uint8>> glyph_buffers;
    vector<stbrp_rect> rects;
    for (uint16 i = 0; i < 128; i++) {
        error = FT_Load_Char(face, i, FT_LOAD_RENDER);
        if (error) {
            glyph_buffers.push_back({});
            continue;
        }

        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (error) {
            continue;
        }

        auto& bitmap = face->glyph->bitmap;
        v2i size = {int32(bitmap.width), int32(bitmap.rows)};
        font.glyphs[i] = Glyph{
            .atlas_position = {}, // Written in write glyphs step
            .atlas_size = size,
            .mesh_offset = {face->glyph->bitmap_left, -face->glyph->bitmap_top},
            .mesh_size = size,
            .mesh_advance = {face->glyph->advance.x / 64, face->glyph->advance.y}
        };
        glyph_buffers.push_back(vector<uint8>{bitmap.buffer, bitmap.buffer + bitmap.width * bitmap.rows});
        rects.push_back(stbrp_rect{
            .id = i,
            .w = size.x + 2,
            .h = size.y + 2
        });
    }

    // pack rects
    stbrp_context rp_context;
    stbrp_node nodes_storage[1000];
    stbrp_init_target(&rp_context, 600, 1000, nodes_storage, 1000);
    auto code = stbrp_pack_rects(&rp_context, rects.data(), rects.size());
    if (code != 1) {
        log_warning("Font atlas packing error");
        FT_Done_Face(face);
        return nullptr;
    }

    // get total image size
    for (const stbrp_rect& rect : rects) {
        texture.size = math::max(texture.size, v2i{rect.x + rect.w, rect.y + rect.h});
    }
    texture.pixels.resize(texture.size.x * texture.size.y);
    font.atlas_size = texture.size;

    // write glyphs
    for (const stbrp_rect& dst_rect : rects) {
        font.glyphs[dst_rect.id].atlas_position = {dst_rect.x + 1, dst_rect.y + 1};
        v2i src_dim = {dst_rect.w - 2, dst_rect.h - 2};

        for (int32 y = 1; y < dst_rect.h - 1; y++) {
            vector<uint8>& atlas_dst = texture.pixels;
            uint32 dst_offset = (dst_rect.y + y) * texture.size.x + (dst_rect.x) + 1;

            vector<uint8>& glyph_src = glyph_buffers[dst_rect.id];
            uint32 src_offset = (y - 1) * src_dim.x;

            memcpy(atlas_dst.data() + dst_offset, glyph_src.data() + src_offset, src_dim.x);
        }
    }

    texture.format = vuk::Format::eR8Unorm;
    texture.needs_mips = !ui;

    upload_texture(texture, false);
    TextureGPU* texture_gpu = get_gpu_asset_cache().get_texture(font.id);

    FT_Done_Face(face);

    if (ui)
        make_ui_material(font.id, *texture_gpu);
    else
        make_3d_text_material(font.id, *texture_gpu);

    return &font;
}

}