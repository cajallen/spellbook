#pragma once

#include "consumer.hpp"
#include "scene.hpp"
#include "enemy.hpp"
#include "spawner.hpp"
#include "tower.hpp"
#include "tile.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct AssetEditor {
    enum Tab {
        Tab_Model,
        Tab_Mesh,
        Tab_Material,
        Tab_Tower,
        Tab_Tile,
        Tab_Enemy,
        Tab_Spawner,
        Tab_Consumer
    };
    
    Scene*   p_scene = nullptr;

    ModelCPU model_cpu;
    MeshCPU mesh_cpu;
    MaterialCPU material_cpu;
    TowerPrefab tower_prefab;
    TilePrefab tile_prefab;
    EnemyPrefab enemy_prefab;
    SpawnerPrefab spawner_prefab;
    ConsumerPrefab consumer_prefab;
    
    // Used as readonly
    Tab tab;
    
    void      setup();
    void      update();
    void      window(bool* p_open);
};

}
