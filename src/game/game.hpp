#pragma once

#include "editor/debug_ui.hpp"

namespace spellbook {

struct Scene;

struct Game {
    vector<Scene*> scenes;
    DebugUI debug_ui;

    void startup();
    void run();
    void step(bool skip_input = false); // skip input is for blocking calls to not recurse (resize)
    void shutdown();
};

extern Game game;

}
