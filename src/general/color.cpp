#include "color.hpp"

#include <imgui.h>
#include <vuk/Types.hpp>
#include "math.hpp"

namespace spellbook {

Color mix(Color c1, Color c2, f32 amt) {
    return Color {c1.r * c1.a * (1.0f - amt) + c2.r * c2.a * amt,
        c1.g * c1.a * (1.0f - amt) + c2.g * c2.a * amt,
        c1.b * c1.a * (1.0f - amt) + c2.b * c2.a * amt,
        c1.a * (1.0f - amt) + c2.a * amt};
}

bool operator==(const Color& lhs, const Color& rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

float srgb2linear(float x) {
    if (x <= 0.0f)
        return 0.0f;
    if (x >= 1.0f)
        return 1.0f;
    if (x < 0.04045f)
        return x / 12.92f;
    return std::pow((x + 0.055f) / 1.055f, 2.4f);
}

float linear2srgb(float x) {
    if (x <= 0.0f)
        return 0.0f;
    if (x >= 1.0f)
        return 1.0f;
    if (x < 0.0031308f)
        return x * 12.92f;
    return std::pow(x, 1.0f / 2.4f) * 1.055f - 0.055f;
}

Color::operator vuk::ClearColor() const {
    return {srgb2linear(r), srgb2linear(g), srgb2linear(b), a};
}

#define COLOR_VIEW(var) { \
    f32 var##copy[4] = { var.r, var.g, var.b, var.a }; \
    ImGui::ColorEdit4(#var, var##copy, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_NoInputs); \
}

using namespace palette;
void color_window(bool* p_open) {
    if (ImGui::Begin("Colors", p_open)) {
        COLOR_VIEW(spellbook_0)
        COLOR_VIEW(spellbook_1)
        COLOR_VIEW(spellbook_2)
        COLOR_VIEW(spellbook_3)
        COLOR_VIEW(spellbook_4)
        COLOR_VIEW(spellbook_5)
        COLOR_VIEW(spellbook_6)
        COLOR_VIEW(spellbook_7)
        COLOR_VIEW(spellbook_8)
        COLOR_VIEW(indian_red)
        COLOR_VIEW(light_coral)
        COLOR_VIEW(salmon)
        COLOR_VIEW(dark_salmon)
        COLOR_VIEW(light_salmon)
        COLOR_VIEW(crimson)
        COLOR_VIEW(red)
        COLOR_VIEW(fire_brick)
        COLOR_VIEW(dark_red)
        COLOR_VIEW(pink)
        COLOR_VIEW(light_pink)
        COLOR_VIEW(hot_pink)
        COLOR_VIEW(deep_pink)
        COLOR_VIEW(medium_violet_red)
        COLOR_VIEW(pale_violet_red)
        COLOR_VIEW(coral)
        COLOR_VIEW(tomato)
        COLOR_VIEW(orange_red)
        COLOR_VIEW(dark_orange)
        COLOR_VIEW(orange)
        COLOR_VIEW(gold)
        COLOR_VIEW(yellow)
        COLOR_VIEW(light_yellow)
        COLOR_VIEW(lemon_chiffon)
        COLOR_VIEW(light_goldenrod_yellow)
        COLOR_VIEW(papaya_whip)
        COLOR_VIEW(moccasin)
        COLOR_VIEW(peach_puff)
        COLOR_VIEW(pale_goldenrod)
        COLOR_VIEW(khaki)
        COLOR_VIEW(dark_khaki)
        COLOR_VIEW(lavender)
        COLOR_VIEW(thistle)
        COLOR_VIEW(plum)
        COLOR_VIEW(violet)
        COLOR_VIEW(orchid)
        COLOR_VIEW(fuchsia)
        COLOR_VIEW(magenta)
        COLOR_VIEW(medium_orchid)
        COLOR_VIEW(medium_purple)
        COLOR_VIEW(rebecca_purple)
        COLOR_VIEW(blue_violet)
        COLOR_VIEW(dark_violet)
        COLOR_VIEW(dark_orchid)
        COLOR_VIEW(dark_magenta)
        COLOR_VIEW(purple)
        COLOR_VIEW(indigo)
        COLOR_VIEW(slate_blue)
        COLOR_VIEW(dark_slate_blue)
        COLOR_VIEW(medium_slate_blue)
        COLOR_VIEW(green_yellow)
        COLOR_VIEW(chartreuse)
        COLOR_VIEW(lawn_green)
        COLOR_VIEW(lime)
        COLOR_VIEW(lime_green)
        COLOR_VIEW(pale_green)
        COLOR_VIEW(light_green)
        COLOR_VIEW(medium_spring_green)
        COLOR_VIEW(spring_green)
        COLOR_VIEW(medium_sea_green)
        COLOR_VIEW(sea_green)
        COLOR_VIEW(forest_green)
        COLOR_VIEW(green)
        COLOR_VIEW(dark_green)
        COLOR_VIEW(yellow_green)
        COLOR_VIEW(olive_drab)
        COLOR_VIEW(olive)
        COLOR_VIEW(dark_olive_green)
        COLOR_VIEW(medium_aquamarine)
        COLOR_VIEW(dark_sea_green)
        COLOR_VIEW(light_sea_green)
        COLOR_VIEW(dark_cyan)
        COLOR_VIEW(teal)
        COLOR_VIEW(aqua)
        COLOR_VIEW(cyan)
        COLOR_VIEW(light_cyan)
        COLOR_VIEW(pale_turquoise)
        COLOR_VIEW(aquamarine)
        COLOR_VIEW(turquoise)
        COLOR_VIEW(medium_turquoise)
        COLOR_VIEW(dark_turquoise)
        COLOR_VIEW(cadet_blue)
        COLOR_VIEW(steel_blue)
        COLOR_VIEW(light_steel_blue)
        COLOR_VIEW(powder_blue)
        COLOR_VIEW(light_blue)
        COLOR_VIEW(sky_blue)
        COLOR_VIEW(light_sky_blue)
        COLOR_VIEW(deep_sky_blue)
        COLOR_VIEW(dodger_blue)
        COLOR_VIEW(cornflower_blue)
        COLOR_VIEW(royal_blue)
        COLOR_VIEW(blue)
        COLOR_VIEW(medium_blue)
        COLOR_VIEW(dark_blue)
        COLOR_VIEW(navy)
        COLOR_VIEW(midnight_blue)
        COLOR_VIEW(cornsilk)
        COLOR_VIEW(blanched_almond)
        COLOR_VIEW(bisque)
        COLOR_VIEW(navajo_white)
        COLOR_VIEW(wheat)
        COLOR_VIEW(burly_wood)
        COLOR_VIEW(tan)
        COLOR_VIEW(rosy_brown)
        COLOR_VIEW(sandy_brown)
        COLOR_VIEW(goldenrod)
        COLOR_VIEW(dark_goldenrod)
        COLOR_VIEW(peru)
        COLOR_VIEW(chocolate)
        COLOR_VIEW(saddle_brown)
        COLOR_VIEW(sienna)
        COLOR_VIEW(brown)
        COLOR_VIEW(maroon)
        COLOR_VIEW(white)
        COLOR_VIEW(snow)
        COLOR_VIEW(honey_dew)
        COLOR_VIEW(mint_cream)
        COLOR_VIEW(azure)
        COLOR_VIEW(alice_blue)
        COLOR_VIEW(ghost_white)
        COLOR_VIEW(white_smoke)
        COLOR_VIEW(sea_shell)
        COLOR_VIEW(beige)
        COLOR_VIEW(old_lace)
        COLOR_VIEW(floral_white)
        COLOR_VIEW(ivory)
        COLOR_VIEW(antique_white)
        COLOR_VIEW(linen)
        COLOR_VIEW(lavender_blush)
        COLOR_VIEW(misty_rose)
        COLOR_VIEW(gainsboro)
        COLOR_VIEW(light_gray)
        COLOR_VIEW(silver)
        COLOR_VIEW(dark_gray)
        COLOR_VIEW(gray)
        COLOR_VIEW(dim_gray)
        COLOR_VIEW(light_slate_gray)
        COLOR_VIEW(slate_gray)
        COLOR_VIEW(dark_slate_gray)
        COLOR_VIEW(near_black)
        COLOR_VIEW(black)
        COLOR_VIEW(gray_1)
        COLOR_VIEW(gray_2)
        COLOR_VIEW(gray_3)
        COLOR_VIEW(gray_4)
        COLOR_VIEW(gray_5)
        COLOR_VIEW(gray_6)
        COLOR_VIEW(gray_7)
        COLOR_VIEW(gray_8)
        COLOR_VIEW(gray_9)
    }
    ImGui::End();
}

}
