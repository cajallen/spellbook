#pragma once

#include "editor_scene.hpp"
#include "game/map.hpp"

namespace spellbook {

// This class should be incredibly simple because it just drives the Scene class
struct GameScene : EditorScene {
    void setup() override { log_error("Game Scene should only be setup through map prefab"); }
    void setup(const MapPrefab& map_prefab);
    void update() override;
    void window(bool* p_open) override;
    void shutdown() override;

    void setup_player_stuff();
};

}
