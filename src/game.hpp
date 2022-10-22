#pragma once

#include "renderer/renderer.hpp"
#include "game/asset_editor.hpp"

#include "string.hpp"
#include "vector.hpp"
#include "gui.hpp"
#include "scene.hpp"


namespace spellbook {

const string general_extension = ".sbgen";

struct Game {
    vector<Scene*> scenes;
    Renderer       renderer;
    GUI            gui;

    // MapEditor editor;
    AssetEditor editor;
    
    string resource_folder = "resources";
    string user_folder = "user";

    void startup();
    void run();
    void step(bool skip_input = false); // skip input is for blocking calls to not recurse (resize)
    void shutdown();
};

extern Game game;

}
