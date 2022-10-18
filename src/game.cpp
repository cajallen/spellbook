#include "game.hpp"

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


namespace spellbook {

Game game;

void Game::startup() {
    Console::setup();
    renderer.setup();
    Input::setup();

    editor.setup();
    scenes.insert_back(editor.p_scene);
    scenes.last()->name = "Test Scene";
    scenes.last()->setup();
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
    editor.update();
    for (Scene* scene : scenes)
        scene->update();
    renderer.render();
    FrameMark;
}

void Game::shutdown() {
    for (Scene* scene : scenes) {
        scene->cleanup();
        delete scene;
    }
    renderer.cleanup();
}

}
