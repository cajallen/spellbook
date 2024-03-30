
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
#include "game/text_3d.hpp"
#include "game/effects/text_layer.hpp"
#include "game/effects/hit.hpp"
#include "game/effects/hit_effect.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/font_manager.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

ADD_EDITOR_SCENE(HitTestScene);

void HitTestScene::setup() {
    setup_scene(new Scene(), false);
    //gui_manager.shop_gui.add_lizard_card<LizardType_Ranger>();
    //gui_manager.shop_gui.add_lizard_card<LizardType_Assassin>();
}

void HitTestScene::setup_scene(Scene* scene, bool scene_setup) {
    p_scene = scene;
    if (!scene_setup)
        p_scene->setup("HitTestScene");
    p_scene->set_edit_mode(false);

    gui_manager.setup(p_scene);

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

    god_caster = p_scene->registry.create();
    p_scene->registry.emplace<Team>(god_caster, Team::Team_Lizards);
    p_scene->registry.emplace<LogicTransform>(god_caster, v3(0.0f, 0.0f, 2.0f));


//    entt::entity text_test = p_scene->registry.create();
//    Text3D& text_cmp = p_scene->registry.emplace<Text3D>(text_test, p_scene);
//    Text3DTransform& text_tfm = p_scene->registry.emplace<Text3DTransform>(text_test, v3(0.5f, 0.5f, 1.0f));
//    text_cmp.set_text("{shadow}{col=ffffff}Hello world!");
//    text_tfm.billboard_mode = Text3DTransform::BillboardMode_Z;
//    //text_tfm.alignment_axis = v3::X;
}

void HitTestScene::update() {
    if (p_scene->pause)
        return;

    Viewport& viewport = p_scene->render_scene.viewport;

    p_scene->update();

    gui_manager.draw();
    gui_manager.update();
}

#define EFFECT_ROW(Type) {ImGui::TableNextRow();\
if (!effects.contains(#Type)) effects[#Type] = {}; \
auto& effect_config = effects[#Type];\
ImGui::TableSetColumnIndex(0);\
ImGui::Text(#Type);\
ImGui::TableSetColumnIndex(1);\
ImGui::Checkbox("##" #Type "Enabled", &effect_config.enabled);\
ImGui::TableSetColumnIndex(2); \
ImGui::SetNextItemWidth(-FLT_MIN); \
ImGui::SliderInt("##" #Type "Level", &effect_config.level, 0, 8);\
ImGui::TableSetColumnIndex(3); \
ImGui::SetNextItemWidth(-FLT_MIN); \
ImGui::DragFloat("##" #Type "Quantity", &effect_config.quantity, 0.1f);\
ImGui::TableSetColumnIndex(4); \
ImGui::SetNextItemWidth(-FLT_MIN); \
ImGui::DragFloat("##" #Type "Duration", &effect_config.duration, 0.01f); \
HitEffect effect = HitEffect(EffectType_##Type);\
effect.level = effect_config.level; effect.duration = effect_config.duration; effect.quantity = effect_config.quantity; \
if (effect_config.enabled) hit.effects.push_back(effect);}

void HitTestScene::window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("HitTestScene", p_open)) {
        Team& god_team = p_scene->registry.get<Team>(god_caster);
        if (ImGui::EnumCombo("Team", &god_team.identity)) {
            god_team = Team(god_team.identity);
        }

        Hit hit;
        hit.scene = p_scene;
        hit.position = v3i{};
        hit.caster = god_caster;
        if (ImGui::BeginTable("Effects", 5)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Use", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Level");
            ImGui::TableSetupColumn("Quantity");
            ImGui::TableSetupColumn("Duration");
            ImGui::TableHeadersRow();

            EFFECT_ROW(Damage);
            EFFECT_ROW(Vulnerable);
            EFFECT_ROW(Weaken);
            EFFECT_ROW(Slow);

            EFFECT_ROW(Heal);
            EFFECT_ROW(Regen);
            EFFECT_ROW(Growth);
            EFFECT_ROW(Resistance);
            EFFECT_ROW(Haste);
            EFFECT_ROW(Amplify);

            ImGui::EndTable();
        }

        ImGui::Spacing();
        if (ImGui::Button("Apply")) {
            hit.process();
        }
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
