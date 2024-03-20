#include "gui_items.hpp"

#include "gui_manager.hpp"
#include "game/effects/text_layer.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

ItemGUI::ItemGUI(GUIManager& manager) : manager(manager) {
}
void ItemGUI::draw() {
}
range2i ItemGUI::get_occupied_region() {
    return range2i(given_region.start, given_region.start);
}

range2i SpacingGUI::get_occupied_region() {
    return range2i{given_region.start, given_region.start + size};
}

void Panel::draw() {
    if (!item) return;

    Renderable r = {};
    r.frame_allocated = true;

    // background
    MeshUICPU background_mesh = generate_rounded_quad(given_region, 32, 2, background_tint, 5.0f, Input::time * 5.0f, false, 1.0f);
    upload_mesh(background_mesh, true);
    r.mesh_id = background_mesh.id;
    r.material_id = make_ui_material(texture_path);
    r.sort_index = depth;
    manager.scene->render_scene.ui_renderables.push_back(r);

    // full outline
    MeshUICPU background_outline_mesh = generate_rounded_outline(given_region, 32, 2, 2.0f, palette::black, 5.0f, Input::time * 5.0f);
    upload_mesh(background_outline_mesh, true);
    r.mesh_id = background_outline_mesh.id;
    r.material_id = make_ui_material("white"_symbolic);
    r.sort_index = depth + 2;
    manager.scene->render_scene.ui_renderables.push_back(r);

    item->given_region = given_region;
    item->depth = depth;
    item->draw();
}

range2i Panel::get_occupied_region() {
    if (!item) return range2i{given_region.start, given_region.end + padding * 2};
    item->given_region = range2i{given_region.start + padding, given_region.end - padding};
    range2i item_region = item->get_occupied_region();
    return range2i{item_region.start - padding, item_region.end + padding};
}

range2i ButtonGUI::get_occupied_region() {
    if (!item) return range2i{given_region.start, given_region.start};
    item->given_region = range2i{given_region.start, given_region.end};
    return item->get_occupied_region();
}

void ImageGUI::set_image(const FilePath& fp, v2i bounds, bool keep_aspect) {
    TextureCPU tex = load_texture(fp);
    size = tex.size;
}
range2i ImageGUI::get_occupied_region() {
    return {given_region.start, given_region.start + size};
}

void ImageGUI::draw() {
    Renderable r = {};
    r.frame_allocated = true;

    MeshUICPU image_mesh = generate_quad(given_region.start, given_region.end, palette::white);
    upload_mesh(image_mesh, true);
    r.mesh_id = image_mesh.id;
    r.material_id = make_ui_material(texture_path);
    r.sort_index = depth;
    manager.scene->render_scene.ui_renderables.push_back(r);
}

void ButtonGUI::draw() {
    if (!item) return;

    item->given_region = given_region;
    item->depth = depth;
    item->draw();
}

void TextGUI::draw() {
    TextSettings text_settings {
        .depth = depth,
        .width = given_region.end.x - given_region.start.x,
        .distortion_amount = 0.0f,
        .ref_floats = ref_floats,
        .ref_strings = ref_strings,
        .hovered_id = &manager.hovered->id,
        .hovered_index = &manager.hovered->char_index,
        .distortion_time = Input::time * 10.0f
    };
    range2i text_region = calc_formatted_text_region(text, given_region.start, text_settings);

    // offset is needed because glyphs start from the bottom
    v2i offset = text_region.start - given_region.start;

    umap<uint64, vector<TooltipRegion>> tooltip_regions;
    upload_formatted_text(text, given_region.start + offset, text_settings, &tooltip_regions, manager.scene->render_scene, true);

    InteractRegion text_interact_region;
    text_interact_region.id = uint64(this);
    text_interact_region.region = {text_region.start + offset, text_region.end + offset};
    text_interact_region.priority = depth;
    text_interact_region.clickable = true;
    manager.add_interact_region(text_interact_region);

    for (auto& [key, regions] : tooltip_regions) {
        for (const auto& region : regions) {
            InteractRegion interact_region {
                .id = region.id,
                .char_index = region.index,
                .priority = depth + 5,
                .region = region.region,
                .clickable = false
            };
            manager.add_interact_region(interact_region);
        }
    }
}
range2i TextGUI::get_occupied_region() {
    TextSettings text_settings {
        .depth = depth,
        .width = given_region.end.x - given_region.start.x,
        .distortion_amount = 0.0f,
        .ref_floats = ref_floats,
        .ref_strings = ref_strings,
        .hovered_id = &manager.hovered->id,
        .hovered_index = &manager.hovered->char_index,
        .distortion_time = Input::time * 10.0f
    };

    range2i text_region = calc_formatted_text_region(text, given_region.start, text_settings);

    // offset is needed because glyphs start from the bottom
    v2i offset = text_region.start - given_region.start;

    text_region.start += offset;
    text_region.end += offset;

    return text_region;
}

void VerticalBox::draw() {
    int32 height = 0;
    vector<range2i> item_regions;
    for (unique_ptr<ItemGUI>& item : items) {
        item_regions.push_back(item->get_occupied_region());
        height += item_regions.back().end.y - item_regions.back().start.y;
    }

    int32 v_cursor = given_region.start.y;
    int32 extra_height = math::max(given_region.end.y - given_region.start.y - height, 0);
    for (uint32 i = 0; i < items.size(); i++) {
        if (i < spacing_weights.size()) {
            v_cursor += spacing_weights[i] * extra_height;
        }
        items[i]->given_region = range2i{item_regions[i].start + v2i{0, v_cursor}, item_regions[i].end + v2i{0, v_cursor}};
        items[i]->depth = depth + 5;
        items[i]->draw();
        v_cursor += item_regions[i].end.y - item_regions[i].start.y;
    }
}

range2i VerticalBox::get_occupied_region() {
    int32 height = 0;
    for (unique_ptr<ItemGUI>& item : items) {
        range2i item_region = item->get_occupied_region();
        height += item_region.end.y - item_region.start.y;
    }

    int extra_height = math::max(given_region.end.y - given_region.start.y - height, 0);
    for (float weight : spacing_weights) {
        height += weight * extra_height;
    }
    if (spacing_weights.size() > items.size()) {
        height += spacing_weights.back() * extra_height;
    }
    return range2i{given_region.start, v2i{given_region.end.x, given_region.start.y + height}};
}

void HorizontalBox::draw() {
    int32 width = 0;
    vector<range2i> item_regions;
    for (unique_ptr<ItemGUI>& item : items) {
        item_regions.push_back(item->get_occupied_region());
        width += item_regions.back().end.x - item_regions.back().start.x;
    }

    int32 h_cursor = given_region.start.x;
    int32 extra_width = math::max(given_region.end.x - given_region.start.x - width, 0);
    for (uint32 i = 0; i < items.size(); i++) {
        if (i < spacing_weights.size()) {
            h_cursor += spacing_weights[i] * extra_width;
        }
        items[i]->given_region = range2i{item_regions[i].start + v2i{h_cursor, 0}, item_regions[i].end + v2i{h_cursor, 0}};
        items[i]->depth = depth + 5;
        items[i]->draw();
        h_cursor += item_regions[i].end.x - item_regions[i].start.x;
    }
}

range2i HorizontalBox::get_occupied_region() {
    int32 width = 0;
    int32 height = 0;
    for (unique_ptr<ItemGUI>& item : items) {
        range2i item_region = item->get_occupied_region();
        width += item_region.end.x - item_region.start.x;
        height = math::max(height, item_region.end.y - item_region.start.y);
    }

    int extra_width = math::max(given_region.end.x - given_region.start.x - width, 0);
    for (float weight : spacing_weights) {
        width += weight * extra_width;
    }
    if (spacing_weights.size() > items.size()) {
        width += spacing_weights.back() * extra_width;
    }
    return range2i{given_region.start, v2i{given_region.start.x + width, given_region.start.y + height}};
}


}