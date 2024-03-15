#pragma once

#include <ft2build.h>
#include <freetype/freetype.h>
#include <vuk/SampledImage.hpp>

#include "general/file/file_path.hpp"
#include "general/math/geometry.hpp"
#include "renderer/renderable.hpp"
#include "renderer/assets/mesh.hpp"

namespace spellbook {

struct RenderScene;

struct FontRequestInfo {
    FilePath path;
    uint32   size;

    bool operator==(const FontRequestInfo& rhs) const {
        return path == rhs.path && size == rhs.size;
    }
};

}

template<>
struct std::hash<spellbook::FontRequestInfo> {
    std::size_t operator()(const spellbook::FontRequestInfo& v) const noexcept {
        return robin_hood::hash<std::string>{}(v.path.value) ^ size_t(v.size);
    }
};

namespace spellbook {

struct Glyph {
    v2i atlas_position;
    v2i atlas_size;
    v2i mesh_offset;
    v2i mesh_size;
    v2i mesh_advance;
};

struct Font {
    uint64 id;
    int32 size;
    std::array<Glyph, 128> glyphs;
    vuk::SampledImage image = vuk::SampledImage(vuk::SampledImage::Global{});
    v2i atlas_size;
};

struct FontManager {
    bool initialized = false;

    FT_Library library;
    umap<FontRequestInfo, Font> fonts;

    void setup();

    Font* load_font(const FilePath& path, uint32 font_size);
};

inline FontManager& get_font_manager() {
    static FontManager font_manager;
    return font_manager;
}

struct TextFormatState {
    bool reading_tag = false;
    string tag = "";
    Color32 color = Color32(palette::gray_8);
    float lift = 0.0f;
    bool shadow = false;
    range2i* tooltip = nullptr;
    bool tooltip_initiated = false;
};

range2i calc_text_region(const vector<string>& strings, const vector<Font*>& fonts, v2i position);
vector<Renderable*> upload_text_mesh(const vector<string>& strings, const vector<Font*>& fonts, v2i position, int32 depth, float distortion_amount, float distortion_time, umap<uint64, vector<range2i>>* tooltip_regions, RenderScene& render_scene, bool frame_allocated = false);

char _read_color_char(string_view s, int8 index);
uint8 _interpret_hex_char(char c);

}