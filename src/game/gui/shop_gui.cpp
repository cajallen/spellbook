#include <renderer/assets/mesh.hpp>
#include "shop_gui.hpp"

#include "gui_manager.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"

#include "gui_items.hpp"

namespace spellbook {


void ShopGUI::draw(GUIManager* manager, RenderScene& render_scene) {
    if (!enabled)
        return;

    render_scene.viewport.start;
    render_scene.viewport.size;
    range2i viewport_region = {render_scene.viewport.start, render_scene.viewport.start + render_scene.viewport.size};

    int32 i = 0;

    v2i position = v2i{viewport_region.start.x, viewport_region.end.y};
    position += {300, -100};
    for (LizardCardGUI& card : cards) {
        card.position = {position.x, position.y - card.size.y};

        card.draw(manager);

        i++;
        position.x += card.size.x;
        position.x += 50;
    }

    if (buy_button_panel) {
        buy_button_panel->given_region = {position + v2i(0, -50), position + v2i(100, 0)};
        buy_button_panel->draw();
    }
}

void ShopGUI::update(GUIManager* manager) {
    float last_selected = -FLT_MAX;
    for (LizardCardGUI& card : cards) {
        card.update(manager);
        last_selected = math::max(last_selected, card.selected_at);
    }
    for (LizardCardGUI& card : cards) {
        if (card.selected_at < last_selected)
            card.selected_at = 0.0f;
    }
    if (last_selected > 0) {
        if (!buy_button_panel) {
            TextGUI text(*manager);
            text.text = fmt_("{{header}}{{shadow}}{{col=ffffff}}Buy{{\\col}}{{\\shadow}}{{\\header}}");

            buy_button_panel = make_unique<PanelGUI>(*manager);
            buy_button_panel->background_tint = palette::burly_wood;
            buy_button_panel->outline_color = mix(palette::burly_wood, palette::black, 0.7f);

            buy_button_panel->depth = 500;
            buy_button_panel->item = make_unique<TextGUI>(std::move(text));

            buy_button_panel->start_padding = {30, 15};
            buy_button_panel->end_padding   = {30, 15};
            buy_button_panel->outline_width = 2.0f;
            buy_button_panel->hoverable = true;
            buy_button_panel->clickable = true;
        }

        if (!manager->pressed)
            return;
        if (manager->pressed->id == buy_button_panel->id) {

        }
    } else if (buy_button_panel) {
        buy_button_panel = {};
    }
}

}