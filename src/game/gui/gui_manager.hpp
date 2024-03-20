#pragma once

#include "general/math/geometry.hpp"
#include "general/math/math.hpp"

#include "card_gui.hpp"
#include "shop_gui.hpp"

namespace spellbook {

struct Scene;
struct TooltipRegion;

struct InteractRegion {
    uint64 id;
    uint32 char_index;
    int32 priority;
    range2i region;

    bool clickable;

    bool operator<(const InteractRegion& rhs) const {
        return priority < rhs.priority;
    }
    bool operator>(const InteractRegion& rhs) const {
        return priority > rhs.priority;
    }
    bool operator==(const InteractRegion& rhs) const {
        return id > rhs.id;
    }
};

struct GUIManager {
    Scene* scene;

    vector<InteractRegion> interact_regions;

    ShopGUI shop_gui;

    InteractRegion* hovered;
    InteractRegion* pressed;

    void setup(Scene* p_scene);
    void update();
    void draw();

    void add_interact_region(const InteractRegion& new_region);
    void remove_interact_region(uint64 id);
};

}