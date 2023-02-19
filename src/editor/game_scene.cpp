﻿#include "game_scene.hpp"

#include "game/game.hpp"
#include "renderer/draw_functions.hpp"

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

bool game_scene_click_dragging(ClickCallbackArgs args) {
    GameScene& game_scene = *((GameScene*) args.data);
    if (game_scene.p_scene->render_scene.viewport.hovered && args.action == GLFW_PRESS) {
        game_scene.p_scene->render_scene.query = v2i(Input::mouse_pos) - game_scene.p_scene->render_scene.viewport.start;
        return true;
    }
    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        game_scene.p_scene->registry.clear<Dragging>();
        game_scene.p_scene->selected_entity = entt::null;
    }
    return false;
}

void GameScene::setup(const MapPrefab& map_prefab) {
    Input::add_callback(InputCallbackInfo{game_scene_on_key, 20, "Game Scene", this});
    Input::add_callback(InputCallbackInfo{game_scene_click_dragging, 50, "Game Scene", this});
    p_scene = instance_map(map_prefab, "Game Scene");
    p_scene->set_edit_mode(false);
    p_scene->time_scale = 1.0f;
    setup_player_stuff();

    auto cube = generate_cube(v3(0.0f, 0.0f, -1.0f), v3(100.0f, 100.0f, 0.5f));
    auto cube_name = upload_mesh(cube);
    auto water_name = upload_material(MaterialCPU{.file_path = "water_mat", .color_tint = palette::dodger_blue});
    p_scene->render_scene.quick_mesh(cube_name, water_name, false);
}

void GameScene::update() {
    p_scene->update();

    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->registry.clear<Dragging>();
        p_scene->selected_entity = entt::null;
    }
}

void GameScene::window(bool* p_open) {

}

void GameScene::shutdown() {
    Input::remove_callback<KeyCallback>("Game Scene");
    p_scene->cleanup();
}

void GameScene::setup_player_stuff() {
    p_scene->player.scene = p_scene;
    p_scene->shop.shop_generator = std::make_unique<SimpleShopGenerator>();
    p_scene->shop.shop_generator->setup();
}



}