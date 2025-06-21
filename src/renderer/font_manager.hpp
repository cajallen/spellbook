#pragma once

#include <array>
#include <vuk/ImageAttachment.hpp>

#include "general/file/file_path.hpp"
#include "general/math/geometry.hpp"

using std::array;

struct FT_LibraryRec;
typedef struct FT_LibraryRec_*  FT_Library;

namespace spellbook {
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
        return spellbook::hash_combine(robin_hood::hash<std::string>{}(v.path.value), uint64(v.size));
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
    array<Glyph, 128> glyphs;
    vuk::SampledImage image = {};
    v2i atlas_size;
};

struct FontManager {
    bool initialized = false;

    FT_Library library;
    umap<FontRequestInfo, Font> fonts;

    void setup();

    Font* load_font(const FilePath& path, uint32 font_size, bool ui = true);
};

inline FontManager& get_font_manager() {
    static FontManager font_manager;
    return font_manager;
}

}