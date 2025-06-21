#include "game_scene.hpp"

#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"
#include "editor/editor_scene.hpp"
#include "game/map.hpp"
#include "game/game.hpp"
#include "game/shop.hpp"
#include "game/player.hpp"
#include "game/gui/gui_manager.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizards/instance_lizard.hpp"

namespace spellbook {

bool game_scene_on_key(KeyCallbackArgs args) {
    GameScene& game_scene = *((GameScene*) args.data);
    if (!game_scene.p_scene->render_scene->viewport.hovered && !game_scene.p_scene->render_scene->viewport.focused)
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
    if (game_scene.p_scene->render_scene->viewport.hovered && args.action == GLFW_PRESS) {
        game_scene.p_scene->render_scene->query.input = v2i(Input::mouse_pos) - game_scene.p_scene->render_scene->viewport.start;
        return true;
    }
    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        game_scene.p_scene->release_selected();
    }
    return false;
}

void GameScene::setup(const MapPrefab& map_prefab) {
    Input::add_callback(InputCallbackInfo{game_scene_on_key, 20, "Game Scene", this});
    Input::add_callback(InputCallbackInfo{game_scene_click_dragging, 50, "Game Scene", this});
    assert_else(p_scene == nullptr)
        delete p_scene;
    p_scene = instance_map(map_prefab, "Game Scene");
    p_scene->set_edit_mode(false);
    p_scene->time_scale = 1.0f;
    setup_player_stuff();

    p_scene->shop->open(p_scene);
    for (const MapPrefab::HomeEntry& home : map_prefab.homes) {
        p_scene->map_data.homes.emplace_back(home.position, home.rotation);
    }
}

void GameScene::update() {
    p_scene->update();

    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->release_selected();
    }
}

void GameScene::window(bool* p_open) {
    if (ImGui::Begin("Game Scene", p_open)) {
        if (ImGui::CollapsingHeader("Lizards")) {
            magic_enum::enum_for_each<LizardType>([&](auto val) {
                constexpr LizardType lizard_type = val;
                auto& lizard = get_lizard_database().lizard_entries.at(lizard_type);

                auto& variant1 = lizard.variants.at(LizardVariant_First);
                if (variant1.title.empty())
                    return;

                string button_name1 = fmt_("{} - {}", lizard.name, variant1.title);
                if (ImGui::Button(button_name1.c_str())) {
                    instance_lizard<lizard_type, LizardVariant_First>(p_scene);
                }

                auto& variant2 = lizard.variants.at(LizardVariant_Second);
                if (variant2.title.empty())
                    return;

                string button_name2 = fmt_("{} - {}", lizard.name, variant2.title);
                if (ImGui::Button(button_name2.c_str())) {
                    instance_lizard<lizard_type, LizardVariant_Second>(p_scene);
                }

                auto& variant3 = lizard.variants.at(LizardVariant_Third);
                if (variant3.title.empty())
                    return;

                string button_name3 = fmt_("{} - {}", lizard.name, variant3.title);
                if (ImGui::Button(button_name3.c_str())) {
                    instance_lizard<lizard_type, LizardVariant_Third>(p_scene);
                }

            });
        }
    }
    ImGui::End();
}

void GameScene::shutdown() {
    Input::remove_callback<KeyCallback>("Game Scene");
    p_scene->cleanup();
}

void GameScene::setup_player_stuff() {
    p_scene->player->scene = p_scene;
    p_scene->shop->shop_generator = make_unique<FirstFreeShopGenerator>();
    p_scene->shop->shop_generator->setup(p_scene);

    p_scene->gui_manager->hud_gui.player = &*p_scene->player;
    p_scene->gui_manager->hud_gui.enabled = false; // don't want to deal with UI right now
}



}
