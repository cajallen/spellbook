#include "card_gui.hpp"

#include "gui_manager.hpp"
#include "extension/fmt.hpp"
#include "general/input.hpp"
#include "general/color.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"
#include "game/effects/text_layer.hpp"
#include "game/gui/gui_items.hpp"


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
    Renderable r      = {};
    r.material_id     = untextured_mat;
    r.frame_allocated = true;

    v2i used_position = calculate_used_position();

    Panel panel(*manager);
    panel.padding      = {20, 20};
    panel.given_region = {used_position, used_position + size};

    VerticalBox card_layout(*manager);
    {
        HorizontalBox title_bar(*manager);
        SpacingGUI    title_left_spacing(*manager);
        title_left_spacing.size = {10, 10};
        TextGUI text(*manager);
        text.text = fmt_("{{header}}{{col={}}}{}{{\\col}}{{\\header}}", lizard.color.to_string(), lizard.name);
        title_bar.items.emplace_back(make_unique<SpacingGUI>(title_left_spacing));
        title_bar.items.emplace_back(make_unique<TextGUI>(text));
        // title_bar.spacing_weights = {0.0f, 0.0f, 1.0f};
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(title_bar)));
    }
    {
        HorizontalBox attack_name_bar(*manager);
        TextGUI       attack_name(*manager);
        attack_name.text        = attack.name;
        attack_name.ref_floats  = &attack.extra_floats;
        attack_name.ref_strings = &attack.extra_strings;
        attack_name_bar.items.emplace_back(make_unique<TextGUI>(std::move(attack_name)));
        card_layout.spacing_weights.push_back(1.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(attack_name_bar)));
    }
    if (expanded == Expanded_Attack) {
        HorizontalBox attack_text_bar(*manager);
        TextGUI       attack_text(*manager);
        attack_text.text        = attack.description;
        attack_text.ref_floats  = &attack.extra_floats;
        attack_text.ref_strings = &attack.extra_strings;
        attack_text_bar.items.emplace_back(make_unique<TextGUI>(std::move(attack_text)));
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(attack_text_bar)));
    }
    {
        SpacingGUI spacing(*manager);
        spacing.size = {5, 5};
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<SpacingGUI>(std::move(spacing)));
    }
    {
        HorizontalBox spell_name_bar(*manager);
        TextGUI spell_name(*manager);
        spell_name.text = spell.name;
        spell_name.ref_floats = &spell.extra_floats;
        spell_name.ref_strings = &spell.extra_strings;
        spell_name_bar.items.emplace_back(make_unique<TextGUI>(std::move(spell_name)));
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(spell_name_bar)));
    }
    if (expanded == Expanded_Spell) {
        HorizontalBox spell_text_bar(*manager);
        TextGUI spell_text(*manager);
        spell_text.text = spell.description;
        spell_text.ref_floats = &spell.extra_floats;
        spell_text.ref_strings = &spell.extra_strings;
        spell_text_bar.items.emplace_back(make_unique<TextGUI>(std::move(spell_text)));
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(spell_text_bar)));
    }

    panel.item = make_unique<VerticalBox>(std::move(card_layout));
    panel.depth = 500;
    panel.background_tint = palette::gray_2;
    panel.draw();
}

void LizardCardGUI::update(GUIManager* manager) {
    if (manager->pressed && manager->pressed->id == uint64(this) && selected_at == 0.0f) {
        selected_at = Input::time;
    }

    if (manager->pressed && manager->pressed->id == uint64(this) + 1) {
        if (expanded == Expanded_Attack) {
            expanded = Expanded_None;
        } else {
            expanded = Expanded_Attack;
        }
    }

    if (manager->pressed && manager->pressed->id == uint64(this) + 2) {
        if (expanded == Expanded_Spell) {
            expanded = Expanded_None;
        } else {
            expanded = Expanded_Spell;
        }
    }

}
v2i LizardCardGUI::calculate_used_position() const {
    v2i used_position = position;
    if (selected_at > 0.0f)
        used_position.y += -math::clamp((Input::time - selected_at) * 8.0f, range{0.0f, 1.0f}) * 50.0f;
    return used_position;
}

}