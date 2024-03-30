#pragma once

#include "general/color.hpp"
#include "general/math/geometry.hpp"
#include "renderer/renderable.hpp"

namespace spellbook {

struct RenderScene;
struct Font;

struct TooltipRegion {
    range2i region;
    uint32 index;
    uint64 id;
};

struct TextFormatState {
    bool reading_tag = false;
    string tag = "";
    Color32 color = Color32(palette::gray_8);
    int32 lift = 0;
    bool shadow = false;
    TooltipRegion* tooltip = nullptr;
    bool tooltip_initiated = false;
};

struct TextSettings {
    bool spatial;

    // non spatial settings
    v2i position;
    int32 depth;
    float distortion_amount;
    float distortion_time;
    umap<uint64, vector<TooltipRegion>>* tooltip_regions;
    uint64* hovered_id;
    uint32* hovered_index;
};

range2i calc_text_region(const vector<string>& strings, const vector<Font*>& fonts, v2i position);
vector<Renderable*> upload_text_mesh(const vector<string>& strings, const vector<Font*>& fonts, RenderScene& render_scene, const TextSettings& text_settings, bool frame_allocated = false);

char _read_color_char(string_view s, int8 index);
uint8 _interpret_hex_char(char c);

}