#include "game.hpp"

#include <filesystem>
#include <tracy/Tracy.hpp>

#include "renderer/render_scene.hpp"
#include "editor/console.hpp"
#include "editor/editor_scene.hpp"
#include "editor/asset_editor.hpp"
#include "editor/map_editor.hpp"
#include "game/input.hpp"
#include "game/scene.hpp"

namespace fs = std::filesystem;

namespace spellbook {

Game game;

void Game::startup() {
    Console::setup();
    renderer.setup();
    Input::setup();

    for (auto editor_scene : EditorScenes::values()) {
        editor_scene->setup();
    }
    
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
    for (auto editor_scene : EditorScenes::values()) {
        editor_scene->update();
    }
    renderer.render();
    FrameMark;
}

void Game::shutdown() {
    gui.shutdown();
    for (auto editor_scene : EditorScenes::values()) {
        editor_scene->shutdown();
    }
    
    while (!scenes.empty()) {
        Scene* scene = scenes.front();
        scene->cleanup();
        delete scene;
    }
    renderer.cleanup();
}

}
