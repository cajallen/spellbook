#include "widget_system.hpp"

#include "game/input.hpp"

namespace spellbook {

bool widget_click(ClickCallbackArgs args) {
    if (args.action == GLFW_PRESS) {
        float closest_depth = FLT_MAX;
        u64 closest_id;
        for (auto& [id, depth] : WidgetSystem::depths) {
            if (depth < closest_depth) {
                closest_depth = depth;
                closest_id = id;
            }
        }
    
        if (closest_depth < FLT_MAX) {
            WidgetSystem::pressed_id = closest_id;
            return true;
        }
    }
    else if (args.action == GLFW_RELEASE) {
        WidgetSystem::pressed_id = 0;
    }
    return false;
}


namespace WidgetSystem {

umap<u64, float> depths;
u64 pressed_id;

void setup() {
    Input::add_callback(InputCallbackInfo{widget_click, 40, "widget_system", nullptr});
}

void update() {
    for (auto& [id, depth] : WidgetSystem::depths) {
        depth = FLT_MAX;
    }
}

}

}
