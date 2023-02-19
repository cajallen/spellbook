#pragma once

#include "editor_scene.hpp"
#include "game/consumer.hpp"
#include "game/scene.hpp"
#include "game/enemy.hpp"
#include "game/spawner.hpp"
#include "game/lizard.hpp"
#include "game/tile.hpp"
#include "game/visual_tile.hpp"

#include "renderer/assets/model.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/mesh.hpp"

namespace fs = std::filesystem;
using std::optional;

namespace spellbook {

struct AssetEditor : EditorScene {
    enum Tab {
        Tab_None,
        Tab_Model,
        Tab_Mesh,
        Tab_Material,
        Tab_Lizard,
        Tab_Tile,
        Tab_Enemy,
        Tab_Spawner,
        Tab_Consumer,
        Tab_Emitter,
        Tab_TileSet,
        Tab_Drop
    };

    bool model_owner;
    ModelCPU* model_cpu;
    
    MeshCPU mesh_cpu;
    Renderable* mesh_gpu;
    
    MaterialCPU material_cpu;
    Renderable* material_gpu;

    EmitterCPU emitter_cpu;
    EmitterGPU* emitter_gpu;
    
    LizardPrefab lizard_prefab;
    TilePrefab tile_prefab;
    EnemyPrefab enemy_prefab;
    SpawnerPrefab spawner_prefab;
    ConsumerPrefab consumer_prefab;

    VisualTileSet tile_set;

    BeadPrefab bead_prefab;

    Tab tab;
    
    void setup() override;
    void update() override;
    void window(bool* p_open) override;
    void shutdown() override;

    void switch_tab(Tab tab);
};

}
