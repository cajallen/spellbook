#pragma once

#include <filesystem>

#include "scene.hpp"
#include "enemy.hpp"
#include "tower.hpp"
#include "tile.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct AssetEditor {
    Scene*   p_scene = nullptr;

    ModelCPU model_cpu;
    MeshCPU mesh_cpu;
    MaterialCPU material_cpu;
    TowerPrefab tower_prefab;
    TilePrefab tile_prefab;
    EnemyPrefab enemy_prefab;
    
    void      setup();
    void      update();
    void      window(bool* p_open);
};

}
