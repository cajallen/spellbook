#include "gui_manager.hpp"

#include "game/scene.hpp"

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "game/effects/text_layer.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

void GUIManager::setup() {
    get_game_text_layer().setup();

    uint64 id = hash_view("untextured_mat");
    auto& tex = get_gpu_asset_cache().get_texture_or_upload("white"_symbolic).value.view;
    vuk::SampledImage image = vuk::make_sampled_image(tex.get(), Sampler().address(Address_Repeat).get());
    make_ui_material(id, image);
}

void GUIManager::update(Scene* p_scene) {
    bool set_hover = false;
    pressed_id = 0;
    range2i hover_region;
    for (int32 i = interact_regions.size() - 1; i >= 0; i--) {
        const InteractRegion& interact_region = interact_regions[i];
        v2i mouse_pos = v2i(Input::mouse_pos) - p_scene->render_scene.viewport.start;
        if (math::contains(interact_region.region, mouse_pos)) {
            if (!set_hover) {
                hovered_id = interact_region.id;
                hover_region = interact_region.region;
                set_hover = true;
            }
            if (Input::mouse_release[0]) {
                if (interact_region.clickable) {
                    pressed_id = hovered_id;
                    break;
                }
                // If it's not clickable, we can still look for something to click
            } else {
                break;
            }
        }
    }

    v2i padding = {20, 20};
    if (get_effect_database().entries.contains(hovered_id)) {
        v2i tooltip_pos = (hover_region.start + hover_region.end) / 2;
        EffectDatabase::EffectEntry& effect_entry = get_effect_database().entries.at(hovered_id);
        string tooltip_text = effect_entry.description;

        TextSettings text_settings {
            .depth = 700 + 5,
            .width = 500,
            .distortion_amount = 0.2f,
            .ref_floats = &effect_entry.extra_floats,
            .ref_strings = &effect_entry.extra_strings,
            .distortion_time = Input::time
        };
        tooltip_pos.y += 30;
        range2i region = calc_formatted_text_region(tooltip_text, tooltip_pos, text_settings);
        v2i text_position = region.start + v2i{0, 15} + padding;

        upload_formatted_text(tooltip_text, text_position, text_settings, nullptr, p_scene->render_scene, true);

        range2i used_region = region;
        used_region.end += padding * 2.0f;

        Renderable r = {};
        r.material_id = hash_view("untextured_mat");
        r.frame_allocated = true;

        // background
        MeshUICPU background_mesh = generate_rounded_quad(used_region, 32, 2, palette::gray_3, 5.0f, Input::time * 5.0f);
        upload_mesh(background_mesh, true);
        r.mesh_id = background_mesh.id;
        r.sort_index = 700;
        p_scene->render_scene.ui_renderables.push_back(r);

        // full outline
        MeshUICPU background_outline_mesh = generate_rounded_outline(used_region, 32, 2, 2.0f, palette::black, 5.0f, Input::time * 5.0f);
        upload_mesh(background_outline_mesh, true);
        r.mesh_id = background_outline_mesh.id;
        r.sort_index = 700 + 5;
        p_scene->render_scene.ui_renderables.push_back(r);
    }

    interact_regions.clear();

    shop_gui.update(this);
}

void GUIManager::draw(RenderScene& render_scene) {
    shop_gui.draw(this, render_scene);
}

void GUIManager::add_interact_region(const InteractRegion& new_region) {
    interact_regions.insert_search(new_region);
}

void GUIManager::remove_interact_region(uint64 id) {
    for (const InteractRegion& region : interact_regions) {
        if (region.id == id) {
            interact_regions.remove_value(region, /*unordered=*/false);
            return;
        }
    }
}




}