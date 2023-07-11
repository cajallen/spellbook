#pragma once

#include "editor_scene.hpp"
#include "game/map.hpp"
#include "game/scene.hpp"
#include "game/entities/tile.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/consumer.hpp"
#include "game/entities/spawner.hpp"
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
        Tab_Drop,
        Tab_Map
    };

    // 3 modes, owned, cache, instanced
    enum Ownership {
        Ownership_Null,
        Ownership_Owned,
        Ownership_From_Cache,
        Ownership_From_Instance
    };
    Ownership model_owner = Ownership_Null;
    ModelCPU* model_cpu = nullptr;
    
    MeshCPU mesh_cpu;
    Renderable* mesh_preview = nullptr;
    
    MaterialCPU material_cpu;
    Renderable* material_preview = nullptr;

    EmitterCPU emitter_cpu;
    EmitterGPU* emitter_gpu = nullptr;
    
    LizardPrefab lizard_prefab;
    bool force_target = false;
    TilePrefab tile_prefab;
    EnemyPrefab enemy_prefab;
    SpawnerPrefab spawner_prefab;
    ConsumerPrefab consumer_prefab;
    MapPrefab map_prefab;

    VisualTileSet tile_set;

    BeadPrefab bead_prefab;

    Tab tab;
    Tab external_tab_selection;

    Renderable* background_renderable = nullptr;
    
    void setup() override;
    void update() override;
    void window(bool* p_open) override;
    void shutdown() override;

    void switch_tab(Tab tab);

    void set_model(ModelCPU* model, Ownership ownership);
};

}
