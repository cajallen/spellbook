#pragma once

#include "scene.hpp"

namespace spellbook {

struct AssetEditor {
    Scene* p_scene;

    void setup();
    void update();
    void window(bool* p_open);
};

}
