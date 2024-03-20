#include "gui_manager.hpp"

#include "game/scene.hpp"

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "game/effects/text_layer.hpp"
#include "renderer/draw_functions.hpp"
#include "game/gui/gui_items.hpp"

namespace spellbook {

void GUIManager::setup(Scene* p_scene) {
    scene = p_scene;
    get_game_text_layer().setup();
}

void GUIManager::update() {
    bool set_hover = false;
    pressed = nullptr;
    hovered = nullptr;
    for (int32 i = interact_regions.size() - 1; i >= 0; i--) {
        InteractRegion& interact_region = interact_regions[i];
        v2i mouse_pos = v2i(Input::mouse_pos) - scene->render_scene.viewport.start;
        if (math::contains(interact_region.region, mouse_pos)) {
            if (!set_hover) {
                hovered = &interact_region;
                set_hover = true;
            }
            if (Input::mouse_release[0]) {
                if (interact_region.clickable) {
                    pressed = hovered;
                    break;
                }
                // If it's not clickable, we can still look for something to click
            } else {
                break;
            }
        }
    }

    v2i padding = {20, 20};

    if (hovered && get_effect_database().entries.contains(hovered->id)) {
        EffectDatabase::EffectEntry& effect_entry = get_effect_database().entries.at(hovered->id);
        v2i tooltip_pos = (hovered->region.start + hovered->region.end) / 2;

        Panel panel(*this);
        panel.padding = {15, 15};
        panel.depth = hovered->priority;
        panel.background_tint = palette::gray_2;
        panel.given_region = {tooltip_pos, tooltip_pos + v2i{400, 600}};
        TextGUI text(*this);
        text.text = effect_entry.description;
        text.ref_floats = &effect_entry.extra_floats;
        text.ref_strings = &effect_entry.extra_strings;
        panel.item = make_unique<TextGUI>(text);

        panel.draw();
    }

    interact_regions.clear();

    shop_gui.update(this);
}

void GUIManager::draw() {
    shop_gui.draw(this, scene->render_scene);
}

void GUIManager::add_interact_region(const InteractRegion& new_region) {
    interact_regions.insert_search(new_region);
}

void GUIManager::remove_interact_region(uint64 id) {
    for (const InteractRegion& region : interact_regions) {
        if (region.id == id) {
            interact_regions.remove_value(region, /*unordered=*/false);
            return;
        }
    }
}




}