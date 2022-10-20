#pragma once

#include <filesystem>

#include "scene.hpp"
#include "tower.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct AssetEditor {
    Scene*   p_scene = nullptr;

    fs::path convert_file = {};
    fs::path load_file    = {};
    fs::path other_file = {};
    
    entt::entity entity;

    TowerPrefab tower_prefab;
    
    void      setup();
    void      update();
    void      window(bool* p_open);
};

}
