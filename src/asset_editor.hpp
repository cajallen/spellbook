#pragma once

#include <filesystem>

#include "scene.hpp"
#include "renderer/assets/prefab.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct AssetEditor {
    fs::path convert_file = {};
    fs::path load_file = {};
    PrefabCPU prefab_cpu = {};
    Scene* p_scene = nullptr;

    void setup();
    void update();
    void window(bool* p_open);
};

}
