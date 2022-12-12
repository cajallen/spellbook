#include "game_scene.hpp"

#include "game/game.hpp"

namespace spellbook {

bool game_scene_on_key(KeyCallbackArgs args) {
    GameScene& game_scene = *((GameScene*) args.data);
    if (!game_scene.p_scene->render_scene.viewport.hovered && !game_scene.p_scene->render_scene.viewport.focused)
        return false;

    if (args.key == GLFW_KEY_ESCAPE && args.action == GLFW_PRESS) {
        game_scene.shutdown();
        EditorScenes::values().remove_value(&game_scene);
        return true;
    }
    return false;
}

void GameScene::setup(const MapPrefab& map_prefab) {
    Input::add_callback(InputCallbackInfo{game_scene_on_key, 20, "Game Scene", this});
    p_scene = instance_map(map_prefab, "Game Scene");
    p_scene->edit_mode = false;
}

void GameScene::update() {
    p_scene->update();
}

void GameScene::window(bool* p_open) {

}

void GameScene::shutdown() {
    Input::remove_callback<KeyCallback>("Game Scene");
    p_scene->cleanup();
}


}
