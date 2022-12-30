#pragma once

#include <GLFW/glfw3.h>

#include "general/umap.hpp"

namespace spellbook {

namespace WidgetSystem {

extern umap<u64, float> depths;
extern u64 pressed_id;

void setup();
void update();

}

}
