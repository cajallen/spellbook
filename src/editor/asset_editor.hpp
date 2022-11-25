#pragma once

#include "game/consumer.hpp"
#include "game/scene.hpp"
#include "game/enemy.hpp"
#include "game/spawner.hpp"
#include "game/tower.hpp"
#include "game/tile.hpp"

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
        Tab_Consumer,
        Tab_Emitter
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

    ParticleEmitter* emitter_instance;
    
    // Used as readonly
    Tab tab;
    
    void      setup();
    void      update();
    void      window(bool* p_open);
};

}
