#include <renderer/assets/mesh.hpp>
#include "shop_gui.hpp"

#include "gui_manager.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/assets/mesh.hpp"
#include "game/effects/text_layer.hpp"

namespace spellbook {


void ShopGUI::add_lizard_card(LizardType type) {
    switch (type) {
        case LizardType_Assassin: return add_lizard_card<LizardType_Assassin>();
        case LizardType_Ranger: return add_lizard_card<LizardType_Ranger>();
    }
}


void ShopGUI::draw(GUIManager* manager, RenderScene& render_scene) {
    render_scene.viewport.start;
    render_scene.viewport.size;
    range2i viewport_region = {render_scene.viewport.start, render_scene.viewport.start + render_scene.viewport.size};

    int32 i = 0;

    v2i position = v2i{viewport_region.start.x, viewport_region.end.y};
    position += {300, -100};
    for (LizardCardGUI& card : cards) {
        card.position = {position.x, position.y - card.size.y};

        card.draw(manager, render_scene);

        i++;
        position.x += card.size.x;
        position.x += 50;
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
}

}