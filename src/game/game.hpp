#pragma once

#include "general/string.hpp"
#include "general/vector.hpp"
#include "game/scene.hpp"
#include "editor/gui.hpp"
#include "editor/asset_editor.hpp"
#include "editor/map_editor.hpp"
#include "editor/test_scene.hpp"
#include "renderer/renderer.hpp"

namespace spellbook {

struct Game {
    vector<Scene*> scenes;
    Renderer       renderer;
    GUI            gui;

    MapEditor map_editor;
    // AssetEditor asset_editor;
    TestScene test_scene;
    
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
