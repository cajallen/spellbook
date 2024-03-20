#pragma once

#include "general/file/file_path.hpp"
#include "general/math/geometry.hpp"
#include "renderer/renderable.hpp"
#include "renderer/font_manager.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/assets/mesh.hpp"
#include "game/effects/hit_effect.hpp"

namespace spellbook {

struct InteractRegion;

struct GameTextLayer {
    Font* default_font;
    Font* default_bold_font;
    Font* title_font;
    Font* header_font;

    void setup();
};

inline GameTextLayer& get_game_text_layer() {
    static GameTextLayer game_text_layer;
    return game_text_layer;
}

struct TextSettings {
    int32 depth = 500;
    int32 width = 500;
    float distortion_amount = 0.0f;
    umap<string, float>* ref_floats;
    umap<string, string>* ref_strings;
    uint64* hovered_id;
    uint32* hovered_index;

    float distortion_time = 0.0f;
};

void insert_newlines(int32& position, string& text, Font* font, const TextSettings& settings);
range2i calc_formatted_text_region(const string& text, v2i position, const TextSettings& settings);
vector<Renderable*> upload_formatted_text(const string& text, v2i position, const TextSettings& settings, umap<uint64, vector<TooltipRegion>>* tooltip_regions, RenderScene& render_scene, bool frame_allocated);

}