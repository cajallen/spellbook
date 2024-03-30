#include "text_draw.hpp"

#include "general/math/noise.hpp"
#include "renderer/font_manager.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/assets/mesh.hpp"

namespace spellbook {

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
void _generate_text_mesh_add_quad(MeshUICPU& mesh, v2i top_left, v2i bottom_right, v2 uv_start, v2 uv_end, Color32 col) {
    uint32 start_index = mesh.vertices.size();
    mesh.vertices.emplace_back(v2(top_left.x, top_left.y), v2(uv_start.x, uv_start.y), col);
    mesh.vertices.emplace_back(v2(bottom_right.x, top_left.y), v2(uv_end.x, uv_start.y), col);
    mesh.vertices.emplace_back(v2(bottom_right.x, bottom_right.y), v2(uv_end.x, uv_end.y), col);
    mesh.vertices.emplace_back(v2(top_left.x, bottom_right.y), v2(uv_start.x, uv_end.y), col);
    mesh.indices.push_back(start_index + 0);
    mesh.indices.push_back(start_index + 1);
    mesh.indices.push_back(start_index + 2);
    mesh.indices.push_back(start_index + 0);
    mesh.indices.push_back(start_index + 2);
    mesh.indices.push_back(start_index + 3);
}

void _read_tag(TextFormatState& state, umap<uint64, vector<TooltipRegion>>* tooltip_regions, uint32 char_index) {
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

vector<Renderable*> upload_text_mesh(const vector<string>& strings, const vector<Font*>& fonts, RenderScene& render_scene, const TextSettings& settings, bool frame_allocated) {
    v2i start_position = settings.position;
    v2i position = start_position;

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
                    _read_tag(state, settings.tooltip_regions, char_index);
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
            if (state.tooltip && settings.hovered_id && state.tooltip->id == *settings.hovered_id) {
                if (settings.hovered_index && char_index == *settings.hovered_index)
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
            v2 offset = settings.spatial ? v2{} : v2{
                settings.distortion_amount * (1.0f - 2.0f * fractal_perlin_noise(v3(char_center, settings.distortion_time), 0.02f, 2, 0.5f, 0)),
                settings.distortion_amount * (1.0f - 2.0f * fractal_perlin_noise(v3(char_center, settings.distortion_time), 0.02f, 2, 0.5f, 1))
            };
            v2i offset_i = math::round_cast(offset);

            _generate_text_mesh_add_quad(mesh_base, char_start + offset_i, char_end + offset_i, uv_start, uv_end, state.color);
            if (state.shadow)
                _generate_text_mesh_add_quad(mesh_shadow, char_start + offset_i + v2i(math::round_cast(font.size * 0.07f) + 1), char_end + offset_i + v2i(math::round_cast(font.size * 0.07f) + 1), uv_start, uv_end, Color32(0, 0, 0, 127));
            position.x += glyph.mesh_advance.x;
        }
        mesh_base.id = hash_combine(hash_view(text), font.id);
        mesh_shadow.id = hash_combine(hash_combine(hash_view(text), font.id), hash_view("shadow"));

        if (!mesh_base.vertices.empty()) {
            upload_mesh(mesh_base, frame_allocated);
            Renderable r = {};
            r.mesh_id = mesh_base.id;
            r.material_id = font.id;
            r.frame_allocated = frame_allocated;
            r.sort_index = settings.depth;

            renderables.push_back(&*render_scene.ui_renderables.emplace(r));
        }
        if (!mesh_shadow.vertices.empty()) {
            upload_mesh(mesh_shadow, frame_allocated);
            Renderable r = {};
            r.mesh_id = mesh_shadow.id;
            r.material_id = font.id;
            r.frame_allocated = frame_allocated;
            r.sort_index = settings.depth - 1;

            renderables.push_back(&*render_scene.ui_renderables.emplace(r));
        }
    }
    return renderables;
}


}