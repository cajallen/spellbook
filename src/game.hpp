#pragma once

#include "renderer/renderer.hpp"
#include "game/asset_editor.hpp"
#include "game/map_editor.hpp"
#include "game/test_scene.hpp"

#include "string.hpp"
#include "vector.hpp"
#include "gui.hpp"
#include "scene.hpp"


namespace spellbook {

struct Game {
    vector<Scene*> scenes;
    Renderer       renderer;
    GUI            gui;

    MapEditor map_editor;
    // AssetEditor asset_editor;
    // TestScene test_scene;
    
    string external_resource_folder;
    string resource_folder;
    string user_folder;

    void startup();
    void run();
    void step(bool skip_input = false); // skip input is for blocking calls to not recurse (resize)
    void shutdown();
};

extern Game game;

}
