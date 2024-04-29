#include "card_gui.hpp"

#include "gui_manager.hpp"
#include "extension/fmt.hpp"
#include "general/input.hpp"
#include "general/color.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"
#include "game/gui/gui_items.hpp"


namespace spellbook {


LizardCardGUI::LizardCardGUI(LizardDatabase::LizardEntry& lizard, uint8 variant, LizardDatabase::AbilityEntry& attack, LizardDatabase::AbilityEntry& spell, int32 depth, std::function<void(Scene*,v3i)> instance_func)
    : variant(variant)
    , lizard(lizard)
    , attack(attack)
    , spell(spell)
    , depth(depth)
    , instance_function(instance_func) {
    size = v2i{350, 400};
    selected_at = 0.0f;
    attack_expanded = false;
    spell_expanded = false;
}

void LizardCardGUI::draw(GUIManager* manager) {
    v2i used_position = calculate_used_position();

    PanelGUI panel(*manager);
    panel.start_padding      = {20, 15};
    panel.end_padding      = {20, 15};
    panel.given_region = {used_position, used_position + size};
    panel.texture_path = lizard.background;
    panel.background_tint = palette::burly_wood;
    panel.outline_color = mix(palette::burly_wood, palette::black, 0.7f);
    panel.outline_width = 2.0f;

    uint32 attack_index = 0;
    uint32 spell_index = 0;

    StackGUI card_stack(*manager);
    AlignmentGUI card_splash_container(*manager);
    {
        ImageGUI card_splash(*manager);
        card_splash.set_image(lizard.variants[variant].splash, v2i(200, 500), true);
        card_splash_container.item = make_unique<ImageGUI>(std::move(card_splash));
    }
    VerticalBox card_layout(*manager);
    {
        HorizontalBox title_bar(*manager);
        title_bar.background_opacity = 0.6f;
        SpacingGUI    title_left_spacing(*manager);
        title_left_spacing.size = {10, 10};
        TextGUI text(*manager);
        text.text = fmt_("{{header}}{{shadow}}{{col=ffffff}}{}{{\\col}}{{\\shadow}}{{\\header}}", lizard.name);
        title_bar.spacing_weights.push_back(0.0f);
        title_bar.items.emplace_back(make_unique<SpacingGUI>(title_left_spacing));
        title_bar.spacing_weights.push_back(0.0f);
        title_bar.items.emplace_back(make_unique<TextGUI>(text));
        title_bar.spacing_weights.push_back(1.0f);
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(title_bar)));
    }
    {
        HorizontalBox title_bar(*manager);
        title_bar.background_opacity = 0.6f;
        SpacingGUI    title_left_spacing(*manager);
        title_left_spacing.size = {10, 10};
        TextGUI text(*manager);
        text.text = fmt_("{{bold}}{{shadow}}{{col={}}}{}{{\\col}}{{\\shadow}}{{\\bold}}", lizard.variants[variant].color.to_string(), lizard.variants[variant].title);
        title_bar.spacing_weights.push_back(0.0f);
        title_bar.items.emplace_back(make_unique<SpacingGUI>(title_left_spacing));
        title_bar.spacing_weights.push_back(0.0f);
        title_bar.items.emplace_back(make_unique<TextGUI>(text));
        title_bar.spacing_weights.push_back(1.0f);
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(title_bar)));
    }

    {
        HorizontalBox attack_name_bar(*manager);
        attack_name_bar.background_opacity = 0.6f;
        ImageGUI attack_icon(*manager);
        attack_icon.set_image("resources/icons/attack.sbatex"_content, v2i{20, 20}, true);
        attack_name_bar.spacing_weights.push_back(0.0f);
        attack_name_bar.items.emplace_back(make_unique<ImageGUI>(std::move(attack_icon)));

        SpacingGUI spacing(*manager);
        spacing.size = {10, 10};
        attack_name_bar.spacing_weights.push_back(0.0f);
        attack_name_bar.items.emplace_back(make_unique<SpacingGUI>(std::move(spacing)));

        TextGUI attack_name(*manager);
        attack_name.text = fmt_("{{bold}}{{shadow}}{{col=ffffff}}{}{{\\col}}{{\\shadow}}{{\\bold}}", attack.name);
        attack_name_bar.spacing_weights.push_back(0.0f);
        attack_name_bar.items.emplace_back(make_unique<TextGUI>(std::move(attack_name)));

        attack_name_bar.spacing_weights.push_back(1.0f);

        attack_name_bar.hoverable = true;
        attack_name_bar.clickable = true;
        attack_index = card_layout.items.size();
        card_layout.spacing_weights.push_back(1.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(attack_name_bar)));
    }
    if (attack_expanded) {
        SpacingGUI spacing(*manager);
        spacing.size = {5, 5};
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<SpacingGUI>(std::move(spacing)));

        HorizontalBox attack_text_bar(*manager);
        attack_text_bar.background_opacity = 0.6f;
        TextGUI attack_text(*manager);
        attack_text.text = attack.description;
        attack_text.ref_floats  = &attack.extra_floats;
        attack_text.ref_strings = &attack.extra_strings;
        attack_text_bar.spacing_weights.push_back(0.0f);
        attack_text_bar.items.emplace_back(make_unique<TextGUI>(std::move(attack_text)));
        attack_text_bar.spacing_weights.push_back(1.0f);
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(attack_text_bar)));
    }
    {
        SpacingGUI spacing(*manager);
        spacing.size = {10, 10};
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<SpacingGUI>(std::move(spacing)));
    }
    {
        HorizontalBox spell_name_bar(*manager);
        spell_name_bar.background_opacity = 0.6f;
        ImageGUI spell_icon(*manager);
        spell_icon.set_image("resources/icons/spell.sbatex"_content, v2i{20, 20}, true);
        spell_name_bar.spacing_weights.push_back(0.0f);
        spell_name_bar.items.emplace_back(make_unique<ImageGUI>(std::move(spell_icon)));

        SpacingGUI spacing(*manager);
        spacing.size = {10, 10};
        spell_name_bar.spacing_weights.push_back(0.0f);
        spell_name_bar.items.emplace_back(make_unique<SpacingGUI>(std::move(spacing)));

        TextGUI spell_name(*manager);
        spell_name.text = fmt_("{{bold}}{{shadow}}{{col=ffffff}}{}{{\\col}}{{\\shadow}}{{\\bold}}", spell.name);
        spell_name_bar.spacing_weights.push_back(0.0f);
        spell_name_bar.items.emplace_back(make_unique<TextGUI>(std::move(spell_name)));
        spell_name_bar.spacing_weights.push_back(1.0f);

        spell_name_bar.hoverable = true;
        spell_name_bar.clickable = true;
        spell_index = card_layout.items.size();
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(spell_name_bar)));
    }
    if (spell_expanded) {
        SpacingGUI spacing(*manager);
        spacing.size = {5, 5};
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<SpacingGUI>(std::move(spacing)));

        HorizontalBox spell_text_bar(*manager);
        spell_text_bar.background_opacity = 0.6f;
        TextGUI spell_text(*manager);
        spell_text.text = spell.description;
        spell_text.ref_floats = &spell.extra_floats;
        spell_text.ref_strings = &spell.extra_strings;
        spell_text_bar.spacing_weights.push_back(0.0f);
        spell_text_bar.items.emplace_back(make_unique<TextGUI>(std::move(spell_text)));
        spell_text_bar.spacing_weights.push_back(1.0f);
        card_layout.spacing_weights.push_back(0.0f);
        card_layout.items.emplace_back(make_unique<HorizontalBox>(std::move(spell_text_bar)));
    }

    panel.hoverable = true;
    panel.clickable = true;
    attack_id = card_layout.items[attack_index]->id;
    spell_id = card_layout.items[spell_index]->id;
    card_id = panel.id;

    card_stack.items.emplace_back(make_unique<AlignmentGUI>(std::move(card_splash_container)));
    card_stack.items.emplace_back(make_unique<VerticalBox>(std::move(card_layout)));
    panel.item = make_unique<StackGUI>(std::move(card_stack));
    panel.depth = 500;
    panel.draw();
}

void LizardCardGUI::update(GUIManager* manager) {
    if (!manager->pressed)
        return;
    if (manager->pressed->id == card_id) {
        if (selected_at == 0.0f)
            selected_at = Input::time;
        else
            selected_at = 0.0f;
    }

    if (manager->pressed->id == attack_id) {
        attack_expanded = !attack_expanded;
    }

    if (manager->pressed->id == spell_id) {
        spell_expanded = !spell_expanded;
    }

}
v2i LizardCardGUI::calculate_used_position() const {
    v2i used_position = position;
    if (selected_at > 0.0f)
        used_position.y += -math::clamp((Input::time - selected_at) * 8.0f, range{0.0f, 1.0f}) * 50.0f;
    return used_position;
}

}