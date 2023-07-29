#pragma once

#include "general/string.hpp"
#include "general/vector.hpp"
#include "game/json_cache.hpp"
#include "editor/gui.hpp"
#include "renderer/renderer.hpp"

namespace spellbook {

struct Scene;

struct Game {
    constexpr static string_view external_resource_folder = "external_resources/";
    constexpr static string_view resource_folder = "resources/";
    constexpr static string_view user_folder = "user/";

    vector<Scene*> scenes;
    Renderer       renderer;
    GUI            gui;
    DiskCache      asset_system;

    void startup();
    void run();
    void step(bool skip_input = false); // skip input is for blocking calls to not recurse (resize)
    void shutdown();
};

extern Game game;

}
