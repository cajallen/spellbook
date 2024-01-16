#pragma once

#include "general/string.hpp"
#include "general/vector.hpp"
#include "editor/gui.hpp"
#include "renderer/renderer.hpp"

namespace spellbook {

struct Scene;

struct Game {
    vector<Scene*> scenes;
    GUI            gui;

    void startup();
    void run();
    void step(bool skip_input = false); // skip input is for blocking calls to not recurse (resize)
    void shutdown();
};

extern Game game;

}
