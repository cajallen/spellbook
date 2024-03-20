#include "font_manager.hpp"

#include <stb_rect_pack.h>
#include <vuk/Context.hpp>

#include "extension/fmt.hpp"
#include "general/logger.hpp"
#include "general/math/noise.hpp"
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

    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/ui.vert"_distributed), "shaders/ui.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/ui.frag"_distributed), "shaders/ui.frag"_distributed.abs_string());
        get_renderer().context->create_named_pipeline("ui", pci);
    }
}


Font* FontManager::load_font(const FilePath& path, uint32 font_size) {
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
            .w = size.x,
            .h = size.y
        });
    }

    // pack rects
    stbrp_context rp_context;
    stbrp_node nodes_storage[1000];
    stbrp_init_target(&rp_context, 500, 1000, nodes_storage, 1000);
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
    for (const stbrp_rect& rect : rects) {
        font.glyphs[rect.id].atlas_position = {rect.x, rect.y};
        for (int32 y = 0; y < rect.h; y++) {
            vector<uint8>& atlas_dst = texture.pixels;
            uint32 dst_offset = (rect.y + y) * texture.size.x + (rect.x);

            vector<uint8>& glyph_src = glyph_buffers[rect.id];
            uint32 src_offset = y * rect.w;

            memcpy(atlas_dst.data() + dst_offset, glyph_src.data() + src_offset, rect.w);
        }
    }

    texture.format = vuk::Format::eR8Srgb;
    texture.needs_mips = false;

    upload_texture(texture, false);
    Sampler sampler = Sampler().address(Address_Clamp).filter(Filter_Nearest).mips(false);
    TextureGPU* texture_gpu = get_gpu_asset_cache().get_texture(font.id);

    font.image = vuk::make_sampled_image(texture_gpu->value.view.get(), sampler.get());

    FT_Done_Face(face);

    make_ui_material(font.id, font.image);

    return &font;
}

range2i calc_text_region(const vector<string>& strings, const vector<Font*>& fonts, v2i position) {
    v2i start_position = position;
    range2i region = {};
    bool region_initialized = false;
    bool reading_tag = false;
    for (uint32 i = 0; i < strings.size(); i++) {
        const string& text = strings[i];
        Font* font = fonts[i];
        for (char c : text) {
            if (reading_tag) {
                if (c == '}') {
                    reading_tag = false;
                    continue;
                }
                continue;
            }
            if (c == '{') {
                reading_tag = true;
                continue;
            }
            if (c == '\n') {
                position.x = start_position.x;
                position.y += font->size;
                continue;
            }
            Glyph& glyph = font->glyphs[c];

            v2i char_start = position + glyph.mesh_offset;
            v2i char_end = char_start + glyph.mesh_size;

            if (!region_initialized) {
                region.start = math::min(char_start, char_end);
                region.end = math::max(char_start, char_end);
                region_initialized = true;
            } else {
                region.start = math::min(region.start, math::min(char_start, char_end));
                region.end = math::max(region.end, math::max(char_start, char_end));
            }

            position.x += glyph.mesh_advance.x;
        }
    }
    return region;
}
inline void _generate_text_mesh_add_quad(MeshUICPU& mesh, v2i top_left, v2i bottom_right, v2 uv_start, v2 uv_end, int32 depth, Color32 col) {
    uint32 start_index = mesh.vertices.size();
    mesh.vertices.emplace_back(v3(top_left.x, top_left.y, depth), v2(uv_start.x, uv_start.y), col);
    mesh.vertices.emplace_back(v3(bottom_right.x, top_left.y, depth), v2(uv_end.x, uv_start.y), col);
    mesh.vertices.emplace_back(v3(bottom_right.x, bottom_right.y, depth), v2(uv_end.x, uv_end.y), col);
    mesh.vertices.emplace_back(v3(top_left.x, bottom_right.y, depth), v2(uv_start.x, uv_end.y), col);
    mesh.indices.push_back(start_index + 0);
    mesh.indices.push_back(start_index + 1);
    mesh.indices.push_back(start_index + 2);
    mesh.indices.push_back(start_index + 0);
    mesh.indices.push_back(start_index + 2);
    mesh.indices.push_back(start_index + 3);
}

inline void _read_tag(TextFormatState& state, umap<uint64, vector<TooltipRegion>>* tooltip_regions, uint32 char_index) {
    if (state.tag.starts_with("col=")) {
        state.color = Color32(state.tag.substr(4));
    } else if (state.tag.starts_with("tip=")) {
        if (tooltip_regions) {
            uint64 tooltip_hash = hash_view(state.tag.substr(4));
            (*tooltip_regions)[tooltip_hash].push_back({.index = char_index, .id = tooltip_hash});
            state.tooltip = &tooltip_regions->at(tooltip_hash).back();
            state.tooltip_initiated = false;
        }
    } else if (state.tag.starts_with("shadow")) {
        state.shadow = true;
    } else if (state.tag.starts_with('\\')) {
        if (state.tag == "\\col") {
            state.color = Color32(palette::gray_8);
        }
        if (state.tag == "\\tip") {
            state.lift = 0.0f;
            state.tooltip = nullptr;
        }
        if (state.tag == "\\shadow") {
            state.shadow = false;
        }
    }
    state.tag.clear();
}

vector<Renderable*> upload_text_mesh(const vector<string>& strings, const vector<Font*>& fonts, v2i position, int32 depth, float distortion_amount, float distortion_time, umap<uint64, vector<TooltipRegion>>* tooltip_regions, uint64* hovered_id, uint32* hovered_index, RenderScene& render_scene, bool frame_allocated) {
    v2i start_position = position;

    vector<Renderable*> renderables;

    uint32 char_index = 0;
    for (uint8 i = 0; i < strings.size(); i++) {
        string_view text = strings[i];
        const Font& font = *fonts[i];

        MeshUICPU mesh_base = {};
        MeshUICPU mesh_shadow = {};

        TextFormatState state = {};
        for (char c : text) {
            if (state.reading_tag) {
                if (c == '}') {
                    _read_tag(state, tooltip_regions, char_index);
                    state.reading_tag = false;
                    continue;
                }
                state.tag.push_back(c);
                continue;
            }
            if (c == '{') {
                state.reading_tag = true;
                continue;
            }
            char_index++;
            if (c == '\n') {
                position.x = start_position.x;
                position.y += font.size;
                continue;
            }
            if (hovered_id && hovered_index && state.tooltip && char_index == *hovered_index && state.tooltip->id == *hovered_id) {
                state.lift += 2;
            }

            const Glyph& glyph = font.glyphs[c];
            v2i char_start = position + glyph.mesh_offset + v2i(0, -state.lift);
            v2i char_end = char_start + glyph.mesh_size;
            if (state.tooltip != nullptr) {
                if (!state.tooltip_initiated) {
                    state.tooltip->region = {char_start, char_end + v2i(0, state.lift)};
                    state.tooltip->index = char_index;
                    state.tooltip_initiated = true;
                } else {
                    math::expand(state.tooltip->region, char_start);
                    math::expand(state.tooltip->region, char_end + v2i(0, state.lift));
                }
            }
            v2 uv_start = v2(glyph.atlas_position) / v2(font.atlas_size);
            v2 uv_end = v2(glyph.atlas_position + glyph.atlas_size) / v2(font.atlas_size);

            v2 char_center = v2(char_start + char_end) / 2.0f;
            v2 offset = {
                distortion_amount * (1.0f - 2.0f * fractal_perlin_noise(v3(char_center, distortion_time), 0.02f, 2, 0.5f, 0)),
                distortion_amount * (1.0f - 2.0f * fractal_perlin_noise(v3(char_center, distortion_time), 0.02f, 2, 0.5f, 1))
            };
            v2i offset_i = math::round_cast(offset);

            _generate_text_mesh_add_quad(mesh_base, char_start + offset_i, char_end + offset_i, uv_start, uv_end, depth, state.color);
            if (state.shadow)
                _generate_text_mesh_add_quad(mesh_shadow, char_start + offset_i + v2i(2), char_end + offset_i + v2i(2), uv_start, uv_end, depth - 1, Color32(0, 0, 0, 127));
            position.x += glyph.mesh_advance.x;
        }
        mesh_base.id = hash_view(text) ^ font.id;
        mesh_shadow.id = hash_view(text) ^ font.id ^ hash_view("shadow");

        if (!mesh_base.vertices.empty()) {
            upload_mesh(mesh_base, frame_allocated);
            Renderable r = {};
            r.mesh_id = mesh_base.id;
            r.material_id = font.id;
            r.frame_allocated = frame_allocated;
            r.sort_index = depth;

            render_scene.ui_renderables.push_back(r);
            renderables.push_back(&render_scene.ui_renderables.back());
        }
        if (!mesh_shadow.vertices.empty()) {
            upload_mesh(mesh_shadow, frame_allocated);
            Renderable r = {};
            r.mesh_id = mesh_shadow.id;
            r.material_id = font.id;
            r.frame_allocated = frame_allocated;
            r.sort_index = depth - 1;

            render_scene.ui_renderables.push_back(r);
            renderables.push_back(&render_scene.ui_renderables.back());
        }
    }
    return renderables;
}

}