#pragma once

#include <filesystem>

#include "scene.hpp"
#include "renderer/assets/model.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct AssetEditor {
    Scene*   p_scene = nullptr;

    fs::path convert_file = {};
    fs::path load_file    = {};

    entt::entity entity;
    
    void      setup();
    void      update();
    void      window(bool* p_open);
};

}
