#pragma once

#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "game/map_editor.hpp"

#include "gui.hpp"
#include "scene.hpp"
#include "slotmap.hpp"

#include "vector.hpp"

namespace spellbook {

struct Game {
    vector<Scene*> scenes;
    Renderer       renderer;
    GUI            gui;

    MapEditor editor;

    void startup();
    void run();
    void step(bool skip_input = false); // skip input is for blocking calls to not recurse (resize)
    void shutdown();
};

extern Game game;

}
