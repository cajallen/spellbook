#include "game_scene.hpp"

namespace spellbook {

void GameScene::setup(const MapPrefab& map_prefab) {
    p_scene = instance_map(map_prefab, "Game Scene");
    p_scene->edit_mode = false;
}

void GameScene::update() {
    p_scene->update();
}

void GameScene::window(bool* p_open) {

}

void GameScene::shutdown() {

}


}