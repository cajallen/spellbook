#include "game.hpp"

#include <filesystem>
#include <tracy/Tracy.hpp>

#include "renderer/render_scene.hpp"
#include "editor/console.hpp"
#include "editor/editor_scene.hpp"
#include "editor/resource_editor.hpp"
#include "editor/map_editor.hpp"
#include "general/input.hpp"
#include "game/scene.hpp"

namespace fs = std::filesystem;

namespace spellbook {

Game game;

void Game::startup() {
    get_filepath_app_name() = "spellbook";
    Console::setup();
    get_renderer().setup();
    Input::setup(get_renderer().window);

    for (auto editor_scene : EditorScenes::values()) {
        editor_scene->setup();
    }
    
    debug_ui.setup();
}

void Game::run() {
    while (!Input::exit_accepted) {
        game.step();
    }
}

void Game::step(bool skip_input) {
    if (!skip_input)
        Input::update();
    get_renderer().update();
    debug_ui.update();
    for (auto editor_scene : EditorScenes::values()) {
        editor_scene->update();
    }
    get_renderer().render();
    FrameMark;
}

void Game::shutdown() {
    debug_ui.shutdown();
    for (auto editor_scene : EditorScenes::values()) {
        editor_scene->shutdown();
    }
    
    while (!scenes.empty()) {
        Scene* scene = scenes.front();
        scene->cleanup();
        delete scene;
    }
    get_renderer().cleanup();
}

}
