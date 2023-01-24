#pragma once

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "editor_scene.hpp"
#include "extension/imgui_extra.hpp"
#include "general/color.hpp"
#include "general/vector.hpp"
#include "general/file.hpp"
#include "game/scene.hpp"
#include "game/consumer.hpp"
#include "game/map.hpp"
#include "game/spawner.hpp"
#include "game/lizard.hpp"
#include "game/tile.hpp"
#include "game/enemy.hpp"
#include "game/components.hpp"
#include "game/visual_tile.hpp"

namespace spellbook {

template <typename T>
struct Button {
    string text;
    Color color;
    string item_path;
};

struct MapEditor : EditorScene {
    MapPrefab map_prefab;

    vector<Button<ConsumerPrefab>> consumer_buttons;
    vector<Button<LizardPrefab>> lizard_buttons;
    vector<Button<TilePrefab>>  tile_buttons;
    vector<Button<SpawnerPrefab>> spawner_buttons;

    bool painting = false;
    
    bool eraser_selected = false;
    u32 selected_consumer = ~0u;
    u32 selected_lizard = ~0u;
    u32 selected_tile  = ~0u;
    u32 selected_spawner = ~0u;

    s32 y_level = 0;

    u32 rotation = 0;

    string vts_path;
    umap<std::array<u8, 8>, vector<string>> visual_tileset;
    vector<entt::entity> visual_map_entities;
    
    void setup() override;
    void update() override;
    void window(bool* p_open) override;
    void shutdown() override;

    void unselect_buttons();

    void draw_preview(v3i cell);

    void build_visuals();
    
    template <typename T>
    void instance_and_write_prefab(const T& t, v3i pos) {
        entt::entity old_tile = p_scene->get(pos, (T*) 0);
        if (old_tile != entt::null) {
            p_scene->registry.destroy(old_tile);
        }
        
        map_prefab.add_prefab(t, pos);
        instance_prefab(p_scene, t, pos);
    }
};

JSON_IMPL_TEMPLATE(template <typename T>, Button<T>, text, color, item_path);

}
