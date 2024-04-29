#include "gui_items.hpp"

#include "extension/fmt_geometry.hpp"
#include "editor/console.hpp"
#include "game/scene.hpp"
#include "game/gui/gui_manager.hpp"
#include "game/effects/text_layer.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/text_draw.hpp"

namespace spellbook {

ItemGUI::ItemGUI(GUIManager& manager) : id(math::random_uint64()), manager(manager), hoverable(false), clickable(false) {
}
void ItemGUI::draw() {
}

void ItemGUI::add_interact_region(range2i region) {
    if (!hoverable)
        return;
    InteractRegion text_interact_region;
    text_interact_region.id = id;
    text_interact_region.region = region;
    text_interact_region.priority = depth;
    text_interact_region.clickable = clickable;
    manager.add_interact_region(text_interact_region);
}

range2i ItemGUI::get_occupied_region() {
    return range2i(given_region.start, given_region.start);
}

range2i SpacingGUI::get_occupied_region() {
    return range2i{given_region.start, given_region.start + size};
}

void PanelGUI::draw() {
    range2i region = get_occupied_region();

    if (show) {
        Renderable r = {};
        r.frame_allocated = true;
        // background
        Color c1 = background_tint;
        Color c2 = mix(background_tint, palette::black, 0.2f);
        v3 vignette = {0.4f, 1.0f, 0.3f};
        MeshUICPU background_mesh = generate_rounded_quad(region, 32, 2, true, 1.0f);
        upload_mesh(background_mesh, true);
        r.mesh_id = background_mesh.id;
        r.material_id = make_panel_material(texture_path, c1, c2, vignette);
        r.sort_index = depth;
        manager.scene->render_scene.ui_renderables.emplace(r);

        // full outline
        MeshUICPU background_outline_mesh = generate_rounded_outline(region, 32, 2, outline_width, outline_color);
        upload_mesh(background_outline_mesh, true);
        r.mesh_id = background_outline_mesh.id;
        r.material_id = make_ui_material("white"_symbolic);
        r.sort_index = depth + HIERARCHY_DEPTH_INCREMENT;
        manager.scene->render_scene.ui_renderables.emplace(r);
    }

    if (item) {
        item->given_region = {region.start + start_padding, region.end - end_padding};
        item->depth = depth + HIERARCHY_DEPTH_INCREMENT;
        item->draw();
    }

    console({.str=fmt_("Drew PanelGUI from {} to {}", region.start, region.end), .frame_tags={"GUI"}});
    add_interact_region(region);
}

range2i PanelGUI::get_occupied_region() {
    if (shrink) {
        item->given_region = {given_region.start + start_padding, given_region.end - end_padding};
        range2i occupied_region = item->get_occupied_region();
        return {occupied_region.start - start_padding, occupied_region.end + end_padding};
    } else {
        return given_region;
    }
}

range2i ButtonGUI::get_occupied_region() {
    if (!item) return range2i{given_region.start, given_region.start};
    item->given_region = range2i{given_region.start, given_region.end};
    return item->get_occupied_region();
}

void ImageGUI::set_image(const FilePath& fp, v2i bounds, bool keep_aspect) {
    texture_path = fp;
    const TextureCPU& tex = load_resource<TextureCPU>(fp);
    tiling = false;
    if (keep_aspect) {
        v2 ratio = v2(tex.size) / v2(bounds);
        ratio /= ratio.x > ratio.y ? ratio.x : ratio.y;
        v2 f_bounds = v2(bounds) * ratio;
        size = math::round_cast(f_bounds);
    } else {
        size = bounds;
    }
}
void ImageGUI::set_image_tiling(const FilePath& fp, float scale) {
    texture_path = fp;
    const TextureCPU& tex = load_resource<TextureCPU>(fp);
    v2 f_size = v2(tex.size) * scale;
    size = math::round_cast(f_size);
    tiling = true;
}
range2i ImageGUI::get_occupied_region() {
    if (tiling)
        return given_region;
    else
        return {given_region.start, given_region.start + size};
}

void ImageGUI::draw() {
    Renderable r = {};
    r.frame_allocated = true;

    range2i region = get_occupied_region();

    MeshUICPU image_mesh = !tiling
        ? generate_quad(region.start, region.end, tint)
        : generate_quad(region.start, region.end, size, tint);
    upload_mesh(image_mesh, true);
    r.mesh_id = image_mesh.id;
    r.material_id = make_ui_material(texture_path);
    r.sort_index = depth;
    manager.scene->render_scene.ui_renderables.emplace(r);

    console({.str=fmt_("Drew Image from {} to {}", region.start, region.end), .frame_tags={"GUI"}});

    add_interact_region(region);
}

void ButtonGUI::draw() {
    if (!item) return;

    item->given_region = given_region;
    item->depth = depth;
    item->draw();

    range2i occupied_region = item->get_occupied_region();
    console({.str=fmt_("Drew Button from {} to {}", occupied_region.start, occupied_region.end), .frame_tags={"GUI"}});

    add_interact_region(occupied_region);
}

void TextGUI::draw() {
    GameTextLayerSettings text_settings {
        .depth = depth,
        .width = given_region.end.x - given_region.start.x + 1, // End is inclusive, width is not
        .ref_floats = ref_floats,
        .ref_strings = ref_strings,
        .hovered_id = &manager.hovered->id,
        .hovered_index = &manager.hovered->char_index
    };
    range2i text_region = calc_formatted_text_region(text, given_region.start, text_settings);

    // offset is needed because glyphs start from the bottom
    v2i offset = text_region.start - given_region.start;
    text_region = {text_region.start - offset, text_region.end - offset};

    umap<uint64, vector<TooltipRegion>> tooltip_regions;
    upload_formatted_text(text, given_region.start - offset, text_settings, &tooltip_regions, manager.scene->render_scene, true);

    console({.str=fmt_("Drew Text from {} to {}", text_region.start, text_region.end), .frame_tags={"GUI"}});

    add_interact_region(text_region);

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
    GameTextLayerSettings text_settings {
        .depth = depth,
        .width = given_region.end.x - given_region.start.x,
        .ref_floats = ref_floats,
        .ref_strings = ref_strings,
        .hovered_id = &manager.hovered->id,
        .hovered_index = &manager.hovered->char_index
    };

    range2i text_region = calc_formatted_text_region(text, given_region.start, text_settings);

    // offset is needed because glyphs start from the bottom
    v2i offset = text_region.start - given_region.start;

    text_region.start -= offset;
    text_region.end -= offset;

    return text_region;
}

void VerticalBox::draw() {
    int32 height = 0;
    vector<range2i> item_regions;
    for (unique_ptr<ItemGUI>& item : items) {
        item->given_region = given_region;
        item_regions.push_back(item->get_occupied_region());
        height += item_regions.back().end.y - item_regions.back().start.y;
    }

    int32 v_cursor = 0;
    int32 extra_height = math::max(given_region.end.y - given_region.start.y - height, 0);
    for (uint32 i = 0; i < items.size(); i++) {
        if (i < spacing_weights.size()) {
            v_cursor += spacing_weights[i] * extra_height;
        }
        items[i]->given_region = range2i{item_regions[i].start + v2i{0, v_cursor}, item_regions[i].end + v2i{0, v_cursor}};
        items[i]->depth = depth + HIERARCHY_DEPTH_INCREMENT;
        items[i]->draw();
        v_cursor += item_regions[i].end.y - item_regions[i].start.y;
    }
    range2i region;
    if (spacing_weights.size() > items.size()) {
        region = given_region;
    } else {
        region = range2i{given_region.start, v2i{given_region.end.x, given_region.start.y + v_cursor}};
    }

    if (background_opacity > 0.0f) {
        range2i background_region = {region.start - v2i(2), region.end + v2i(2)};
        Renderable r = {};
        r.frame_allocated = true;
        // background
        MeshUICPU background_mesh = generate_rounded_quad(background_region, 4, 1, Color(v3(0.0f), background_opacity));
        upload_mesh(background_mesh, true);
        r.mesh_id = background_mesh.id;
        r.material_id = make_ui_material("white"_symbolic);
        r.sort_index = depth;
        manager.scene->render_scene.ui_renderables.emplace(r);
    }

    console({.str=fmt_("Drew VBox from {} to {}", region.start, region.end), .frame_tags={"GUI"}});
    add_interact_region(region);
}

range2i VerticalBox::get_occupied_region() {
    int32 height = 0;
    for (unique_ptr<ItemGUI>& item : items) {
        item->given_region = given_region;
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
    int32 height = 0;
    vector<range2i> item_regions;
    for (unique_ptr<ItemGUI>& item : items) {
        item->given_region = given_region;
        item_regions.push_back(item->get_occupied_region());
        width += item_regions.back().end.x - item_regions.back().start.x;
        height = math::max(height, item_regions.back().end.y - item_regions.back().start.y);
    }

    int32 h_cursor = 0;
    int32 extra_width = math::max(given_region.end.x - given_region.start.x - width, 0);
    for (uint32 i = 0; i < items.size(); i++) {
        if (i < spacing_weights.size()) {
            h_cursor += spacing_weights[i] * extra_width;
        }
        items[i]->given_region = range2i{item_regions[i].start + v2i{h_cursor, 0}, item_regions[i].end + v2i{h_cursor, 0}};
        items[i]->depth = depth + HIERARCHY_DEPTH_INCREMENT;
        items[i]->draw();
        h_cursor += item_regions[i].end.x - item_regions[i].start.x;
    }

    range2i region;
    if (spacing_weights.size() > items.size()) {
        region = range2i{given_region.start, v2i{given_region.end.x, given_region.start.y + height}};
    } else {
        region = range2i{given_region.start, v2i{given_region.start.x + h_cursor, given_region.start.y + height}};
    }

    if (background_opacity > 0.0f) {
        range2i background_region = {region.start - v2i(2), region.end + v2i(2)};
        Renderable r = {};
        r.frame_allocated = true;
        // background
        MeshUICPU background_mesh = generate_rounded_quad(background_region, 4, 1, Color(v3(0.0f), background_opacity));
        upload_mesh(background_mesh, true);
        r.mesh_id = background_mesh.id;
        r.material_id = make_ui_material("white"_symbolic);
        r.sort_index = depth;
        manager.scene->render_scene.ui_renderables.emplace(r);
    }

    console({.str=fmt_("Drew HBox from {} to {}", region.start, region.end), .frame_tags={"GUI"}});
    add_interact_region(region);
}

range2i HorizontalBox::get_occupied_region() {
    int32 width = 0;
    int32 height = 0;
    for (unique_ptr<ItemGUI>& item : items) {
        item->given_region = given_region;
        range2i item_region = item->get_occupied_region();
        width += item_region.end.x - item_region.start.x;
        height = math::max(height, item_region.end.y - item_region.start.y);
    }

    int extra_width = math::max(given_region.end.x - given_region.start.x - width, 0);
    for (float weight : spacing_weights) {
        width += weight * extra_width;
    }
    return range2i{given_region.start, v2i{given_region.start.x + width, given_region.start.y + height}};
}

range2i VerticalSeparator::get_occupied_region() {
    return {given_region.start, {given_region.end.x, given_region.start.y + height}};
}

void VerticalSeparator::draw() {
    vector<FormattedVertex2D> vertices;

    v2i p1 = given_region.start + v2i(0, height / 2);
    v2i p2 = v2i(given_region.end.x, given_region.start.y) + v2i(0, height / 2);

    Renderable r = {};
    r.frame_allocated = true;

    // background
    MeshUICPU background_mesh = generate_separator(p1, p2, 5, width, color);
    upload_mesh(background_mesh, true);
    r.mesh_id = background_mesh.id;
    r.material_id = make_ui_material("white"_symbolic);
    r.sort_index = depth;
    manager.scene->render_scene.ui_renderables.emplace(r);

    range2i region = get_occupied_region();
    console({.str=fmt_("Drew VSeparator from {} to {}", region.start, region.end), .frame_tags={"GUI"}});
    add_interact_region(region);
}

range2i StackGUI::get_occupied_region() {
    if (items.empty())
        return {given_region.start, given_region.start};

    range2i region;
    bool region_initialized = false;
    for (unique_ptr<ItemGUI>& item : items) {
        item->given_region = given_region;
        range2i item_region = item->get_occupied_region();
        if (!region_initialized) {
            region = item_region;
            region_initialized = true;
        } else {
            math::expand(region, item_region.start);
            math::expand(region, item_region.end);
        }
    }

    return region;
}

void StackGUI::draw() {
    int32 current_depth = depth;
    for (unique_ptr<ItemGUI>& item : items) {
        current_depth += 2;
        item->given_region = given_region;
        item->depth = current_depth;
        item->draw();
    }

    range2i region = get_occupied_region();
    console({.str=fmt_("Drew HBox from {} to {}", region.start, region.end), .frame_tags={"GUI"}});
    add_interact_region(region);
}

range2i AlignmentGUI::get_occupied_region() {
    return given_region;
}

void AlignmentGUI::draw() {
    if (!item)
        return;
    item->depth = depth;

    item->given_region = given_region;
    range2i item_region = item->get_occupied_region();
    v2i region_size = given_region.end - given_region.start;
    v2i item_size = item_region.end - item_region.start;
    v2i spare_size = region_size - item_size;
    v2i padding = spare_size / 2;
    item->given_region = {given_region.start + padding, given_region.start + item_size};
    item->draw();

    console({.str=fmt_("Drew Alignment from {} to {}", given_region.start, given_region.end), .frame_tags={"GUI"}});
    add_interact_region(given_region);
}

}