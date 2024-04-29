#include "hud_gui.hpp"

#include "gui_manager.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/assets/mesh.hpp"
#include "game/entities/drop.hpp"
#include "game/player.hpp"
#include "game/gui/gui_items.hpp"


namespace spellbook {


void HUDGUI::draw(GUIManager* manager, RenderScene& render_scene) {
    if (!enabled)
        return;
    if (!player)
        return;

    render_scene.viewport.start;
    render_scene.viewport.size;
    range2i viewport_region = {render_scene.viewport.start, render_scene.viewport.start + render_scene.viewport.size};

    int32 i = 0;

    v2i position = v2i{viewport_region.start.x, viewport_region.end.y};
    position += {50, -400};
    v2i size = {200, 300};

    PanelGUI panel(*manager);
    panel.start_padding      = {15, 15};
    panel.end_padding      = {15, 15};
    panel.depth      = 500;
    panel.background_tint = palette::gray_2;
    panel.given_region = {position, position + size};

    VerticalBox vbox(*manager);
    vbox.given_region =  panel.given_region;

    for (int32 bead = 0; bead < Bead_Count; bead++) {
        Bead bead_type = Bead(bead);
        int32 bead_count = player->bank.beads[bead];

        HorizontalBox hbox(*manager);

        TextGUI text_type(*manager);
        text_type.text = fmt_("{}", magic_enum::enum_name(bead_type));
        SpacingGUI    title_left_spacing(*manager);
        title_left_spacing.size = {10, 10};
        TextGUI text_count(*manager);
        text_count.text = fmt_("{}", bead_count);

        hbox.spacing_weights.push_back(0.0f);
        hbox.items.emplace_back(make_unique<TextGUI>(text_type));
        hbox.spacing_weights.push_back(0.0f);
        hbox.items.emplace_back(make_unique<SpacingGUI>(title_left_spacing));
        hbox.spacing_weights.push_back(1.0f);
        hbox.items.emplace_back(make_unique<TextGUI>(text_count));

        vbox.spacing_weights.push_back(0.0f);
        vbox.items.emplace_back(make_unique<HorizontalBox>(std::move(hbox)));
    }

    vbox.depth = 500;
    panel.item = make_unique<VerticalBox>(std::move(vbox));
    panel.draw();


}

void HUDGUI::update(GUIManager* manager) {
}

}