#include <renderer/assets/mesh.hpp>
#include "shop_gui.hpp"

#include "gui_manager.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/assets/mesh.hpp"
#include "game/effects/text_layer.hpp"

namespace spellbook {


void ShopGUI::add_lizard_card(LizardType type) {
    switch (type) {
        case LizardType_Assassin: return add_lizard_card<LizardType_Assassin>();
        case LizardType_Ranger: return add_lizard_card<LizardType_Ranger>();
    }
}


void ShopGUI::draw(GUIManager* manager, RenderScene& render_scene) {
    render_scene.viewport.start;
    render_scene.viewport.size;
    range2i viewport_region = {render_scene.viewport.start, render_scene.viewport.start + render_scene.viewport.size};

    int32 i = 0;

    v2i position = v2i{viewport_region.start.x, viewport_region.end.y};
    position += {300, -100};
    for (LizardCardGUI& card : cards) {
        card.position = {position.x, position.y - card.size.y};

        card.draw(manager, render_scene);

        i++;
        position.x += card.size.x;
        position.x += 50;
    }


    Renderable r = {};
    r.material_id = hash_view("untextured_mat");
    r.frame_allocated = true;


    v2i used_position = position;

    TextSettings text_settings {
        .depth = 500 + 5,
        .width = 1000,
        .distortion_amount = 3.0f,
        .distortion_time = Input::time * 10.0f
    };

    string button_text = "{header}Next Round{\\header}";
    range2i name_text_region = calc_formatted_text_region(button_text, used_position, text_settings);
    int32 text_height = name_text_region.end.y - name_text_region.start.y;
    v2i name_text_offset = v2i(20, -20 - text_height);
    upload_formatted_text(button_text, used_position + name_text_offset, text_settings, nullptr, render_scene, true);
    name_text_region.start += name_text_offset;
    name_text_region.end += name_text_offset;

    range2i used_region = name_text_region;
    used_region.start -= v2i{20, 20};
    used_region.end += v2i{20, 20};

    // background
    MeshUICPU background_mesh = generate_rounded_quad(used_region, 32, 2, palette::gray_3, 5.0f, Input::time * 5.0f);
    upload_mesh(background_mesh, true);
    r.mesh_id = background_mesh.id;
    r.sort_index = 500;
    render_scene.ui_renderables.push_back(r);

    // full outline
    MeshUICPU background_outline_mesh = generate_rounded_outline(used_region, 32, 2, 2.0f, palette::black, 5.0f, Input::time * 5.0f);
    upload_mesh(background_outline_mesh, true);
    r.mesh_id = background_outline_mesh.id;
    r.sort_index = 500 + 5;
    render_scene.ui_renderables.push_back(r);
}

void ShopGUI::update(GUIManager* manager) {
    float last_selected = -FLT_MAX;
    for (LizardCardGUI& card : cards) {
        card.update(manager);
        last_selected = math::max(last_selected, card.selected_at);
    }
    for (LizardCardGUI& card : cards) {
        if (card.selected_at < last_selected)
            card.selected_at = 0.0f;
    }
}

}