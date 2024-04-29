#pragma once

#include "card_gui.hpp"

namespace spellbook {

struct GUIManager;

struct ShopGUI {
    bool enabled;
    vector<LizardCardGUI> cards;

    void draw(GUIManager* manager, RenderScene& render_scene);
    void update(GUIManager* manager);
};

}