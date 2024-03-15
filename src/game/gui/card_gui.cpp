#include "card_gui.hpp"

#include "gui_manager.hpp"
#include "extension/fmt.hpp"
#include "general/input.hpp"
#include "general/color.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"
#include "game/effects/text_layer.hpp"


namespace spellbook {

LizardCardGUI::LizardCardGUI(LizardDatabase::LizardEntry& lizard, LizardDatabase::AbilityEntry& attack, LizardDatabase::AbilityEntry& spell, int32 depth, const std::function<void(Scene*,v3i)>& instance_func)
    : lizard(lizard)
    , attack(attack)
    , spell(spell)
    , depth(depth)
    , instance_function(instance_func) {
    untextured_mat = hash_view("untextured_mat");
    size = v2i{300, 350};
    selected_at = 0.0f;
    expanded = Expanded_None;
}

void LizardCardGUI::draw(GUIManager* manager, RenderScene& render_scene) const {
    Renderable r = {};
    r.material_id = untextured_mat;
    r.frame_allocated = true;

    v2i used_position = calculate_used_position();

    // background
    MeshUICPU background_mesh = generate_rounded_quad(range2i{used_position, used_position + size}, 32, 2, palette::gray_3, 5.0f, Input::time * 5.0f);
    upload_mesh(background_mesh, true);
    r.mesh_id = background_mesh.id;
    r.sort_index = depth;
    render_scene.ui_renderables.push_back(r);

    // full outline
    MeshUICPU background_outline_mesh = generate_rounded_outline(range2i{used_position, used_position + size}, 32, 2, 2.0f, palette::black, 5.0f, Input::time * 5.0f);
    upload_mesh(background_outline_mesh, true);
    r.mesh_id = background_outline_mesh.id;
    r.sort_index = depth + 5;
    render_scene.ui_renderables.push_back(r);


    TextSettings text_settings {
        .depth = depth + 5,
        .width = size.x - 2 * settings.padding.x,
        .distortion_amount = 2.0f,
        .ref_floats = &lizard.extra_floats,
        .ref_strings = &lizard.extra_strings,
        .distortion_time = Input::time * 10.0f
    };
    // title text
    string modified_name_text = fmt_("{{header}}{{col={}}}{}{{\\col}}{{\\header}}", lizard.color.to_string(), lizard.name);
    range2i name_text_region = calc_formatted_text_region(modified_name_text, used_position, text_settings);
    v2i name_text_offset = v2i(25, 10 + name_text_region.end.y - name_text_region.start.y);

    umap<uint64, vector<range2i>> tooltip_regions;
    upload_formatted_text(modified_name_text, used_position + name_text_offset, text_settings, &tooltip_regions, render_scene, true);

    // attack text
    text_settings.ref_floats = &attack.extra_floats;
    text_settings.ref_strings = &attack.extra_strings;
    v2i attack_title_offset = calculate_attack_title_offset();
    upload_formatted_text(attack.name, used_position + attack_title_offset, text_settings, &tooltip_regions, render_scene, true);

    if (expanded == Expanded_Attack) {
        v2i attack_text_offset = calculate_attack_text_offset();
        upload_formatted_text(attack.description, used_position + attack_text_offset, text_settings, &tooltip_regions, render_scene, true);
    }

    // spell text
    text_settings.ref_floats = &spell.extra_floats;
    text_settings.ref_strings = &spell.extra_strings;
    v2i spell_title_offset = calculate_spell_title_offset();
    upload_formatted_text(spell.name, used_position + spell_title_offset, text_settings, &tooltip_regions, render_scene, true);

    if (expanded == Expanded_Spell) {
        v2i spell_text_offset = calculate_spell_text_offset();
        upload_formatted_text(spell.description, used_position + spell_text_offset, text_settings, &tooltip_regions, render_scene, true);
    }

    for (auto& [key, regions] : tooltip_regions) {
        for (const auto& region : regions) {
            InteractRegion interact_region {
                .id = key,
                .priority = depth + 5,
                .region = region,
                .clickable = false
            };
            manager->add_interact_region(interact_region);
        }
    }
}

void LizardCardGUI::update(GUIManager* manager) {
    if (manager->pressed_id == uint64(this) && selected_at == 0.0f) {
        selected_at = Input::time;
    }

    if (manager->pressed_id == uint64(this) + 1) {
        if (expanded == Expanded_Attack) {
            expanded = Expanded_None;
        } else {
            expanded = Expanded_Attack;
        }
    }

    if (manager->pressed_id == uint64(this) + 2) {
        if (expanded == Expanded_Spell) {
            expanded = Expanded_None;
        } else {
            expanded = Expanded_Spell;
        }
    }

    v2i used_position = calculate_used_position();

    InteractRegion card_interact_region;
    // TODO: not suitable
    card_interact_region.id = uint64(this);
    card_interact_region.region = {used_position, used_position + size};
    card_interact_region.priority = depth;
    card_interact_region.clickable = true;
    manager->add_interact_region(card_interact_region);

    InteractRegion attack_interact_region;
    // TODO: not suitable
    attack_interact_region.id = uint64(this) + 1;
    attack_interact_region.region = calculate_attack_title_region();
    attack_interact_region.priority = depth + 1;
    attack_interact_region.clickable = true;
    manager->add_interact_region(attack_interact_region);

    InteractRegion spell_interact_region;
    // TODO: not suitable
    spell_interact_region.id = uint64(this) + 2;
    spell_interact_region.region = calculate_spell_title_region();
    spell_interact_region.priority = depth + 1;
    spell_interact_region.clickable = true;
    manager->add_interact_region(spell_interact_region);

}
v2i LizardCardGUI::calculate_used_position() const {
    v2i used_position = position;
    if (selected_at > 0.0f)
        used_position.y += -math::clamp((Input::time - selected_at) * 8.0f, range{0.0f, 1.0f}) * 50.0f;
    return used_position;
}


range2i LizardCardGUI::calculate_attack_title_region() const {
    v2i used_position = calculate_used_position();
    range2i attack_text_region = calc_formatted_text_region(attack.name, used_position, {.width = 1000});
    v2i attack_text_offset = calculate_attack_title_offset();
    attack_text_region.start += attack_text_offset;
    attack_text_region.end += attack_text_offset;
    return attack_text_region;
}
range2i LizardCardGUI::calculate_spell_title_region() const {
    v2i used_position = calculate_used_position();
    range2i spell_text_region = calc_formatted_text_region(spell.name, used_position, {.width = 1000});
    v2i spell_text_offset = calculate_spell_title_offset();
    spell_text_region.start += spell_text_offset;
    spell_text_region.end += spell_text_offset;
    return spell_text_region;
}

v2i LizardCardGUI::calculate_attack_title_offset() const {
    v2i attack_text_offset = v2i(settings.padding.x, size.y - settings.padding.y - 2 * settings.line_spacing);
    if (expanded == Expanded_Attack || expanded == Expanded_Spell) {
        attack_text_offset.y -= settings.expand_size;
    }
    return attack_text_offset;
}
v2i LizardCardGUI::calculate_spell_title_offset() const {
    v2i spell_text_offset = v2i(settings.padding.x, size.y - settings.padding.y - settings.line_spacing);
    if (expanded == Expanded_Spell) {
        spell_text_offset.y -= settings.expand_size;
    }
    return spell_text_offset;
}

v2i LizardCardGUI::calculate_attack_text_offset() const {
    v2i attack_title_offset = calculate_attack_title_offset();
    return attack_title_offset + v2i{0, settings.line_spacing};
}
v2i LizardCardGUI::calculate_spell_text_offset() const {
    v2i spell_title_offset = calculate_spell_title_offset();
    return spell_title_offset + v2i{0, settings.line_spacing};
}

}