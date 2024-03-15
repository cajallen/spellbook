#include "text_layer.hpp"

#include "hit_effect.hpp"

namespace spellbook {

void GameTextLayer::setup() {
    get_font_manager().setup();

    default_font = get_font_manager().load_font("resources/fonts/t.ttf"_content, 20);
    default_bold_font = get_font_manager().load_font("resources/fonts/t-bold.ttf"_content, 20);
    title_font = get_font_manager().load_font("resources/fonts/dh.otf"_content, 35);
    header_font = get_font_manager().load_font("resources/fonts/t-bold.ttf"_content, 25);
}

inline void _change_font(vector<string>& strings, vector<Font*>& fonts, Font* font) {
    strings.emplace_back();
    fonts.push_back(font);
}
inline void _read_tag(TextFormatState& state, vector<string>& strings, vector<Font*>& fonts, umap<string, float>* ref_floats, umap<string, string>* ref_strings) {
    if (state.tag.starts_with("reg")) {
        _change_font(strings, fonts, get_game_text_layer().default_font);
    } else if (state.tag.starts_with("bold")) {
        _change_font(strings, fonts, get_game_text_layer().default_bold_font);
    } else if (state.tag.starts_with("title")) {
        _change_font(strings, fonts, get_game_text_layer().title_font);
    } else if (state.tag.starts_with("header")) {
        _change_font(strings, fonts, get_game_text_layer().header_font);
    } else if (state.tag.starts_with("tip=")) {
        uint64 tooltip_hash = hash_view(state.tag.substr(4));
        string insertion_string = fmt_("{{col={}}}", Color32(get_effect_database().entries.at(tooltip_hash).color).to_string());
        strings.back().insert(strings.back().size(), insertion_string);
    } else if (state.tag.starts_with("ref=")) {
        string ref_input = state.tag.substr(4);
        if (ref_floats->contains(ref_input)) {
            string insertion_string = fmt_("{:.0f}", ref_floats->at(ref_input));
            strings.back().insert(strings.back().size(), insertion_string);
        } else if (ref_strings->contains(ref_input)) {
            strings.back().insert(strings.back().size(), ref_strings->at(ref_input));
        }
    } else if (state.tag.starts_with('\\')) {
        if (state.tag == "\\bold") {
            _change_font(strings, fonts, get_game_text_layer().default_font);
        }
        if (state.tag == "\\title") {
            _change_font(strings, fonts, get_game_text_layer().default_font);
        }
        if (state.tag == "\\header") {
            _change_font(strings, fonts, get_game_text_layer().default_font);
        }
        if (state.tag == "\\tip") {
            string insertion_string = fmt_("{{\\col}}");
            strings.back().insert(strings.back().size(), insertion_string);
        }
    }
    state.tag.clear();
}

range2i calc_formatted_text_region(const string& text, v2i position, const TextSettings& settings) {
    TextFormatState state = {};

    vector<string> strings;
    vector<Font*> fonts;

    strings.emplace_back();
    fonts.push_back(get_game_text_layer().default_font);

    for (int32 i = 0; i < text.size(); i++) {
        char c = text[i];
        strings.back().push_back(c);
        if (state.reading_tag) {
            if (c == '}') {
                _read_tag(state, strings, fonts, settings.ref_floats, settings.ref_strings);
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
    }

    // Add newlines
    {
        int32 current_position = 0;
        for (uint32 string_index = 0; string_index < strings.size(); string_index++) {
            insert_newlines(current_position, strings[string_index], fonts[string_index], settings);
        }
    }

    return calc_text_region(strings, fonts, position);
}

void insert_newlines(int32& position, string& text, Font* font, const TextSettings& settings) {
    bool reading_tag = false;
    int32 last_acceptable_word_break = -1;
    bool replace_word_break = false;

    for (uint32 text_index = 0; text_index < text.size(); text_index++) {
        char c = text[text_index];
        if (reading_tag) {
            if (c == '}') {
                reading_tag = false;
            }
            continue;
        }
        if (c == '{') {
            reading_tag = true;
            continue;
        }
        if (c == '\n') {
            position = 0;
            continue;
        }
        if (c == ' ') {
            last_acceptable_word_break = text_index;
            replace_word_break = true;
        }
        Glyph& glyph = font->glyphs[c];

        position += glyph.mesh_advance.x;
        if (position > settings.width && last_acceptable_word_break >= 0) {
            if (replace_word_break) {
                text[last_acceptable_word_break] = '\n';
                text_index = last_acceptable_word_break;
            } else {
                text.insert(last_acceptable_word_break + 1, 1, '\n');
                text_index = last_acceptable_word_break + 1;
            }
            position = 0;
        }
    }
}

vector<Renderable*> upload_formatted_text(const string& input_text, v2i position, const TextSettings& settings, umap<uint64, vector<range2i>>* tooltip_regions, RenderScene& render_scene, bool frame_allocated) {
    TextFormatState state = {};

    vector<string> strings;
    vector<Font*> fonts;

    // Split into fonts
    {
        strings.emplace_back();
        fonts.push_back(get_game_text_layer().default_font);

        for (int32 i = 0; i < input_text.size(); i++) {
            char c = input_text[i];
            strings.back().push_back(c);
            if (state.reading_tag) {
                if (c == '}') {
                    _read_tag(state, strings, fonts, settings.ref_floats, settings.ref_strings);
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
        }
    }

    // Add newlines
    {
        int32 current_position = 0;
        for (uint32 string_index = 0; string_index < strings.size(); string_index++) {
            insert_newlines(current_position, strings[string_index], fonts[string_index], settings);
        }
    }

    return upload_text_mesh(strings, fonts, position, settings.depth, settings.distortion_amount, settings.distortion_time, tooltip_regions, render_scene, frame_allocated);
}

}