#pragma once

#include "scene.hpp"
#include "renderer/assets/prefab.hpp"

namespace spellbook {

struct AssetEditor {
    PrefabCPU prefab_cpu;
    Scene* p_scene;

    void setup();
    void update();
    void window(bool* p_open);
};

}
