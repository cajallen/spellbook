#pragma once

#include "editor_scene.hpp"
#include "game/consumer.hpp"
#include "game/scene.hpp"
#include "game/enemy.hpp"
#include "game/spawner.hpp"
#include "game/lizard.hpp"
#include "game/tile.hpp"
#include "game/visual_tile.hpp"

namespace fs = std::filesystem;
using std::optional;

namespace spellbook {

/*
 * The big question here is if we want to change the values of CPU structs,
 * we need to update the GPU structs to display the changes immediately.
 *
 * But, we don't want to completely reupload every time, so we need to have conservative changes.
 * We could also just reupload every second or so, but this will come with some issues, and we
 * should do conservative changes for the following...
 * - Material, because it's easy
 * - Emitter, partially because it's easy, and partially because it's important
 * - Model, mostly with changing transforms
 * - *Skeletons*, open model to change skeletonCPU and see updates immediately.
 *
 * So, figure out how the link works. Which do we inspect? The asset editor here
 * has access to both, but do we want access to it in the map editor? What will
 * the map editor have access to? Do we want to inspect it and change it while the
 * game is running?
 *
 * Figure out skeleton editing.
 * I think it should be something like a simple timeline with keyframes for each bone.
 * You can scrub through the timeline, and when you pause on keyframes, keyed bones will
 * be selected. Can add/remove selection at keyframe, and add keyframes.
 * Animations probably loop on a per-skeleton loop to avoid complicated cycles management,
 *
 * There should probably be two types of animations; looping and transition. These
 * probably have to be explicit, even though it doesn't seem like it because
 * of the switch that's going to happen from pose interpolation to keyframes.
 * 
 * Finally, add a SkeletonController component that takes anim requests by strings.
 * e.g. Dragging/SkeletonController view, play "transition_to_dangling" or "dangling" anim
 */

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
    
    ModelCPU model_cpu;
    
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
