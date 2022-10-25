#include "game.hpp"

#include <filesystem>

#include <vulkan/vulkan.h>

#include <tracy/Tracy.hpp>

#include "matrix_math.hpp"
#include "console.hpp"
#include "input.hpp"
#include "math.hpp"
#include "geometry.hpp"

#include "renderer/render_scene.hpp"
#include "renderer/viewport.hpp"
#include "renderer/camera.hpp"
#include "renderer/utils.hpp"


namespace fs = std::filesystem;

namespace spellbook {

Game game;

void Game::startup() {
    external_resource_folder = (fs::current_path() / "external_resources").string();
    resource_folder = (fs::current_path() / "resources").string();
    user_folder = (fs::current_path() / "user").string();

    Console::setup();
    renderer.setup();
    Input::setup();

    map_editor.setup();
    asset_editor.setup();
    scenes.insert_back(map_editor.p_scene);
    scenes.last()->name = "Map Edit Scene";
    scenes.last()->setup();
    scenes.insert_back(asset_editor.p_scene);
    scenes.last()->name = "Asset Scene";
    scenes.last()->setup();

    gui.setup();
}

void Game::run() {
    while (!Input::exit_accepted) {
        game.step();
    }
}

void Game::step(bool skip_input) {
    if (!skip_input)
        Input::update();
    renderer.update();
    gui.update();
    map_editor.update();
    asset_editor.update();
    for (Scene* scene : scenes)
        scene->update();
    renderer.render();
    FrameMark;
}

void Game::shutdown() {
    gui.shutdown();
    for (Scene* scene : scenes) {
        scene->cleanup();
        delete scene;
    }
    renderer.cleanup();
}

}
