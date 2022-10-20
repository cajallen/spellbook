#pragma once

#include <filesystem>

#include "renderer/renderer.hpp"
#include "game/asset_editor.hpp"

#include "gui.hpp"
#include "scene.hpp"

#include "vector.hpp"

namespace spellbook {

struct Game {
    vector<Scene*> scenes;
    Renderer       renderer;
    GUI            gui;

    // MapEditor editor;
    AssetEditor editor;
    
    std::filesystem::path resource_folder = "resources";

    void startup();
    void run();
    void step(bool skip_input = false); // skip input is for blocking calls to not recurse (resize)
    void shutdown();
};

extern Game game;

}
