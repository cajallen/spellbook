#pragma once

#include "extension/imgui_extra.hpp"
#include "general/color.hpp"
#include "general/vector.hpp"
#include "editor/editor_scene.hpp"
#include "game/scene.hpp"
#include "game/map.hpp"
#include "game/entities/consumer.hpp"
#include "game/entities/spawner.hpp"
#include "game/entities/tile.hpp"
#include "game/visual_tile.hpp"

namespace spellbook {

template <typename T>
struct Button {
    string text;
    Color color;
    FilePath item_path;
};

struct MapEditor : EditorScene {
    MapPrefab map_prefab;

    vector<Button<ConsumerPrefab>> consumer_buttons;
    vector<Button<TilePrefab>>  tile_buttons;
    vector<Button<SpawnerPrefab>> spawner_buttons;

    bool painting = false;
    bool last_paint = false;
    
    bool eraser_selected = false;
    uint32 selected_consumer = ~0u;
    uint32 selected_tile  = ~0u;
    uint32 selected_spawner = ~0u;

    int32 z_level = 0;

    uint32 rotation = 0;

    FilePath vts_path;
    umap<VisualTileCorners, vector<FilePath>> visual_tileset;
    
    void setup() override;
    void setup_scene(Scene* scene, bool scene_setup);
    void update() override;
    void window(bool* p_open) override;
    void shutdown() override;

    void unselect_buttons();

    void draw_preview(v3i cell);

    void build_visuals(Scene* scene, v3i* tile);
    
    void instance_and_write_consumer(const FilePath& path, v3i input_pos);
    void instance_and_write_spawner(const FilePath& path, v3i input_pos);
    void instance_and_write_tile(const FilePath& path, v3i input_pos, uint32 rotation = 0);
};

JSON_IMPL_TEMPLATE(template <typename T>, Button<T>, text, color, item_path);

}
