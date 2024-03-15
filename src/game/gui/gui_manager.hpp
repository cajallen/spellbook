#pragma once

#include "general/math/geometry.hpp"
#include "general/math/math.hpp"

#include "card_gui.hpp"
#include "shop_gui.hpp"

namespace spellbook {

struct Scene;

struct InteractRegion {
    uint64 id;
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
    vector<InteractRegion> interact_regions;

    ShopGUI shop_gui;

    uint64 hovered_id;
    uint64 pressed_id;

    void setup();
    void update(Scene* p_scene);
    void draw(RenderScene& render_scene);

    void add_interact_region(const InteractRegion& new_region);
    void remove_interact_region(uint64 id);
};

}