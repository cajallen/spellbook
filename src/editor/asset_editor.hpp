#pragma once

#include "editor_scene.hpp"
#include "game/consumer.hpp"
#include "game/scene.hpp"
#include "game/enemy.hpp"
#include "game/spawner.hpp"
#include "game/lizard.hpp"
#include "game/tile.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct AssetEditor : EditorScene {
    enum Tab {
        Tab_Model,
        Tab_Mesh,
        Tab_Material,
        Tab_Lizard,
        Tab_Tile,
        Tab_Enemy,
        Tab_Spawner,
        Tab_Consumer,
        Tab_Emitter
    };

    ModelCPU model_cpu;
    MeshCPU mesh_cpu;
    MaterialCPU material_cpu;
    LizardPrefab lizard_prefab;
    TilePrefab tile_prefab;
    EnemyPrefab enemy_prefab;
    SpawnerPrefab spawner_prefab;
    ConsumerPrefab consumer_prefab;

    EmitterGPU* emitter_instance;
    
    // Used as readonly
    Tab tab;
    
    void setup() override;
    void update() override;
    void window(bool* p_open) override;
};

}
