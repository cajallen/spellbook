#pragma once

#include "general/memory.hpp"

#include "card_gui.hpp"
#include "gui_items.hpp"

namespace spellbook {

struct GUIManager;

struct ShopGUI {
    bool enabled;
    vector<LizardCardGUI> cards;
    unique_ptr<PanelGUI> buy_button_panel;

    void draw(GUIManager* manager, RenderScene& render_scene);
    void update(GUIManager* manager);
};

}