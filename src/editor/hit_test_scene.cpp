
#include "hit_test_scene.hpp"

#include <tracy/Tracy.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <renderer/gpu_asset_cache.hpp>

#include "general/math/matrix_math.hpp"
#include "general/math/math.hpp"
#include "game/scene.hpp"
#include "game/entities/tile.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/components.hpp"
#include "game/visual_tile.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/font_manager.hpp"
#include "game/effects/text_layer.hpp"

#include "renderer/draw_functions.hpp"

#include "pose_widget.hpp"

namespace spellbook {

ADD_EDITOR_SCENE(HitTestScene);

void HitTestScene::show_buttons() {

}

void HitTestScene::setup() {
    setup_scene(new Scene(), false);

    gui_manager.setup();
    gui_manager.shop_gui.add_lizard_card<LizardType_Ranger>();
    gui_manager.shop_gui.add_lizard_card<LizardType_Assassin>();
}

void HitTestScene::setup_scene(Scene* scene, bool scene_setup) {
    p_scene = scene;
    if (!scene_setup)
        p_scene->setup("HitTestScene");
    p_scene->set_edit_mode(true);

    TilePrefab tile_prefab = load_resource<TilePrefab>("resources/tiles/empty_tile"_content);
    TilePrefab slot_prefab = load_resource<TilePrefab>("resources/tiles/empty_slot"_content);
    for (int32 x = -2; x <= 2; x++) {
        for (int32 y = -2; y <= 2; y++) {
            for (int32 z = -3; z <= -1; z++) {
                v3i pos = v3i(x, y, z);
                if (-1 <= x && x <= 1 && -1 <= y && y <= 1 && !(x == 0 && y == 0)) {
                    map_prefab.solid_tiles[pos] = 0b100;
                    map_prefab.tiles[pos] = {"resources/tiles/empty_tile"_content, 0};
                    instance_prefab(p_scene, tile_prefab, pos, 0);
                } else {
                    map_prefab.solid_tiles[pos] = 0b010;
                    map_prefab.tiles[pos] = {"resources/tiles/empty_slot"_content, 0};
                    instance_prefab(p_scene, slot_prefab, pos, 0);
                }
            }
        }
    }

    build_visuals(p_scene);


    instance_prefab(p_scene, load_resource<EnemyPrefab>("resources/enemies/innate_bot.sbjenm"_content), v3i(0,0,0));

}

void HitTestScene::update() {
    if (p_scene->pause)
        return;

    Viewport& viewport = p_scene->render_scene.viewport;

    p_scene->update();

    gui_manager.draw(p_scene->render_scene);
    gui_manager.update(p_scene);
}

void HitTestScene::window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("HitTestScene", p_open)) {
        ImGui::InputTextMultiline("Text", &text, ImVec2{500, 100});
    }
    ImGui::End();
}

void HitTestScene::build_visuals(Scene* scene) {
    auto visual_tileset = convert_to_entry_pool(load_resource<VisualTileSet>("resources/visual_tile_sets/desert.sbjvts"_content));
    auto visual_tiles = build_visual_tiles(map_prefab.solid_tiles, visual_tileset, nullptr);
    for (auto& [pos, tile_entry] : visual_tiles) {
        if (scene->visual_map_entities.contains(pos)) {
            scene->registry.destroy(scene->visual_map_entities[pos]);
            scene->visual_map_entities.erase(pos);
        }

        if (!tile_entry.model_path.is_file())
            continue;

        auto entity = scene->registry.create();
        scene->visual_map_entities[pos] = entity;

        StaticModel& model = scene->registry.emplace<StaticModel>(entity,
            instance_static_model(scene->render_scene, load_resource<ModelCPU>(tile_entry.model_path))
        );

        m44GPU tfm = m44GPU(
            math::translate(v3(pos) + v3(1.0f)) *
            math::rotation(quat(v3::Z, tile_entry.rotation.yaw * math::PI * 0.5f)) *
            math::scale(v3(tile_entry.rotation.flip_x ? -1.0f : 1.0f, 1.0f, tile_entry.rotation.flip_z ? -1.0f : 1.0f))
        );
        for (StaticRenderable* renderable : model.renderables) {
            renderable->transform = tfm;
        }
    }
}

}
