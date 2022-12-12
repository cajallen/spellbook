#pragma once

#include "json.hpp"
#include "geometry.hpp"
#include "math.hpp"

struct ImVec4;
namespace vuk {
struct ClearColor;
}

namespace spellbook {

struct Color {
    union {
        struct {
            f32 r, g, b, a;
        };
        v3  rgb;
        v4  rgba;
        f32 data[4];
    };

    Color() {
        r = g = b = a = 1.0f;
    }
    explicit Color(const v4& v) {
        memcpy(data, v.data, sizeof(f32) * 4);
    }
    explicit constexpr Color(const v3& v) {
        rgb = v;
        a = 1.0f;
    }
    explicit constexpr Color(const v3& v, f32 alpha) {
        rgb = v;
        a = alpha;
    }
    explicit constexpr Color(const Color& c, f32 alpha) {
        rgb = c.rgb;
        a = alpha;
    }
    explicit constexpr Color(f32 value) {
        r = g = b = value;
        a         = 1.0f;
    }
    explicit constexpr Color(f32 value, f32 alpha) {
        r = g = b = value;
        a         = alpha;
    }
    explicit constexpr Color(f32 r, f32 g, f32 b) : r(r), g(g), b(b) {
        a = 1.0f;
    }
    explicit constexpr Color(f32 r, f32 g, f32 b, f32 a) : r(r), g(g), b(b), a(a) {}
    explicit constexpr Color(u32 num) {
        r = f32((num >> 3 * 8) & 0xFF) / 255.0f;
        g = f32((num >> 2 * 8) & 0xFF) / 255.0f;
        b = f32((num >> 1 * 8) & 0xFF) / 255.0f;
        a = f32((num >> 0 * 8) & 0xFF) / 255.0f;
    }
    explicit operator u32() const {
        u32 num = 0;
        num |= u32(r * 255.0f) << (3 * 8);
        num |= u32(g * 255.0f) << (2 * 8);
        num |= u32(b * 255.0f) << (1 * 8);
        num |= u32(a * 255.0f) << (0 * 8);
        return num;
    }

    explicit operator v3() const {
        return v3 {r, g, b};
    }
    explicit operator v4() const {
        return v4 {r, g, b, a};
    }
    explicit operator vuk::ClearColor() const;

    static Color hsv(float h, float s, float v, float a = 1.0f) {
        v3 rgb = math::clamp(
            math::abs((v3(h/360.f*6.f)+v3(0.f,4.f,2.f)) % 6.f - 3.f)-v3(1.f), 
            v3(0.f), 
            v3(1.f)
        );

        return Color(v * math::mix(v3(1.f), rgb, s), a);
    }
};

JSON_IMPL(Color, r, g, b, a);

bool operator==(const Color& lhs, const Color& rhs);
Color mix(Color c1, Color c2, f32 amt);
void color_window(bool* p_open);
float srgb2linear(float x);
float linear2srgb(float x);

namespace palette {

const Color x(1.0f, 0.5f, 0.5f);
const Color y(0.5f, 1.0f, 0.5f);
const Color z(0.5f, 0.5f, 1.0f);

const Color gray_1 = Color(0.1f, 0.1f, 0.1f, 1.0f);
const Color gray_2 = Color(0.2f, 0.2f, 0.2f, 1.0f);
const Color gray_3 = Color(0.3f, 0.3f, 0.3f, 1.0f);
const Color gray_4 = Color(0.4f, 0.4f, 0.4f, 1.0f);
const Color gray_5 = Color(0.5f, 0.5f, 0.5f, 1.0f);
const Color gray_6 = Color(0.6f, 0.6f, 0.6f, 1.0f);
const Color gray_7 = Color(0.7f, 0.7f, 0.7f, 1.0f);
const Color gray_8 = Color(0.8f, 0.8f, 0.8f, 1.0f);
const Color gray_9 = Color(0.9f, 0.9f, 0.9f, 1.0f);

const Color spellbook_0 = Color::hsv(310.f + 15.f, 0.1f, 1.0f);
const Color spellbook_1 = Color::hsv(305.f + 15.f, 0.2f, 0.9f);
const Color spellbook_2 = Color::hsv(300.f + 15.f, 0.3f, 0.8f);
const Color spellbook_3 = Color::hsv(295.f + 15.f, 0.4f, 0.7f);
const Color spellbook_4 = Color::hsv(290.f + 15.f, 0.5f, 0.6f);
const Color spellbook_5 = Color::hsv(285.f + 15.f, 0.6f, 0.5f);
const Color spellbook_6 = Color::hsv(280.f + 15.f, 0.7f, 0.4f);
const Color spellbook_7 = Color::hsv(275.f + 15.f, 0.8f, 0.3f);
const Color spellbook_8 = Color::hsv(270.f + 15.f, 0.9f, 0.2f);


const Color spellbook_gray = mix(Color(palette::spellbook_3), Color(palette::gray_5), 0.50f);

// https://htmlcolorcodes.com/color-names/
constexpr Color indian_red(205.f/255.f, 92.f/255.f, 92.f/255.f);
constexpr Color light_coral(240.f/255.f, 128.f/255.f, 128.f/255.f);
constexpr Color salmon(250.f/255.f, 128.f/255.f, 114.f/255.f);
constexpr Color dark_salmon(233.f/255.f, 150.f/255.f, 122.f/255.f);
constexpr Color light_salmon(255.f/255.f, 160.f/255.f, 122.f/255.f);
constexpr Color crimson(220.f/255.f, 20.f/255.f, 60.f/255.f);
constexpr Color red(255.f/255.f, 0.f/255.f, 0.f/255.f);
constexpr Color fire_brick(178.f/255.f, 34.f/255.f, 34.f/255.f);
constexpr Color dark_red(139.f/255.f, 0.f/255.f, 0.f/255.f);
constexpr Color pink(255.f/255.f, 192.f/255.f, 203.f/255.f);
constexpr Color light_pink(255.f/255.f, 182.f/255.f, 193.f/255.f);
constexpr Color hot_pink(255.f/255.f, 105.f/255.f, 180.f/255.f);
constexpr Color deep_pink(255.f/255.f, 20.f/255.f, 147.f/255.f);
constexpr Color medium_violet_red(199.f/255.f, 21.f/255.f, 133.f/255.f);
constexpr Color pale_violet_red(219.f/255.f, 112.f/255.f, 147.f/255.f);
constexpr Color coral(255.f/255.f, 127.f/255.f, 80.f/255.f);
constexpr Color tomato(255.f/255.f, 99.f/255.f, 71.f/255.f);
constexpr Color orange_red(255.f/255.f, 69.f/255.f, 0.f/255.f);
constexpr Color dark_orange(255.f/255.f, 140.f/255.f, 0.f/255.f);
constexpr Color orange(255.f/255.f, 165.f/255.f, 0.f/255.f);
constexpr Color gold(255.f/255.f, 215.f/255.f, 0.f/255.f);
constexpr Color yellow(255.f/255.f, 255.f/255.f, 0.f/255.f);
constexpr Color light_yellow(255.f/255.f, 255.f/255.f, 224.f/255.f);
constexpr Color lemon_chiffon(255.f/255.f, 250.f/255.f, 205.f/255.f);
constexpr Color light_goldenrod_yellow(250.f/255.f, 250.f/255.f, 210.f/255.f);
constexpr Color papaya_whip(255.f/255.f, 239.f/255.f, 213.f/255.f);
constexpr Color moccasin(255.f/255.f, 228.f/255.f, 181.f/255.f);
constexpr Color peach_puff(255.f/255.f, 218.f/255.f, 185.f/255.f);
constexpr Color pale_goldenrod(238.f/255.f, 232.f/255.f, 170.f/255.f);
constexpr Color khaki(240.f/255.f, 230.f/255.f, 140.f/255.f);
constexpr Color dark_khaki(189.f/255.f, 183.f/255.f, 107.f/255.f);
constexpr Color lavender(230.f/255.f, 230.f/255.f, 250.f/255.f);
constexpr Color thistle(216.f/255.f, 191.f/255.f, 216.f/255.f);
constexpr Color plum(221.f/255.f, 160.f/255.f, 221.f/255.f);
constexpr Color violet(238.f/255.f, 130.f/255.f, 238.f/255.f);
constexpr Color orchid(218.f/255.f, 112.f/255.f, 214.f/255.f);
constexpr Color fuchsia(255.f/255.f, 0.f/255.f, 255.f/255.f);
constexpr Color magenta(255.f/255.f, 0.f/255.f, 255.f/255.f);
constexpr Color medium_orchid(186.f/255.f, 85.f/255.f, 211.f/255.f);
constexpr Color medium_purple(147.f/255.f, 112.f/255.f, 219.f/255.f);
constexpr Color rebecca_purple(102.f/255.f, 51.f/255.f, 153.f/255.f);
constexpr Color blue_violet(138.f/255.f, 43.f/255.f, 226.f/255.f);
constexpr Color dark_violet(148.f/255.f, 0.f/255.f, 211.f/255.f);
constexpr Color dark_orchid(153.f/255.f, 50.f/255.f, 204.f/255.f);
constexpr Color dark_magenta(139.f/255.f, 0.f/255.f, 139.f/255.f);
constexpr Color purple(128.f/255.f, 0.f/255.f, 128.f/255.f);
constexpr Color indigo(75.f/255.f, 0.f/255.f, 130.f/255.f);
constexpr Color slate_blue(106.f/255.f, 90.f/255.f, 205.f/255.f);
constexpr Color dark_slate_blue(72.f/255.f, 61.f/255.f, 139.f/255.f);
constexpr Color medium_slate_blue(123.f/255.f, 104.f/255.f, 238.f/255.f);
constexpr Color green_yellow(173.f/255.f, 255.f/255.f, 47.f/255.f);
constexpr Color chartreuse(127.f/255.f, 255.f/255.f, 0.f/255.f);
constexpr Color lawn_green(124.f/255.f, 252.f/255.f, 0.f/255.f);
constexpr Color lime(0.f/255.f, 255.f/255.f, 0.f/255.f);
constexpr Color lime_green(50.f/255.f, 205.f/255.f, 50.f/255.f);
constexpr Color pale_green(152.f/255.f, 251.f/255.f, 152.f/255.f);
constexpr Color light_green(144.f/255.f, 238.f/255.f, 144.f/255.f);
constexpr Color medium_spring_green(0.f/255.f, 250.f/255.f, 154.f/255.f);
constexpr Color spring_green(0.f/255.f, 255.f/255.f, 127.f/255.f);
constexpr Color medium_sea_green(60.f/255.f, 179.f/255.f, 113.f/255.f);
constexpr Color sea_green(46.f/255.f, 139.f/255.f, 87.f/255.f);
constexpr Color forest_green(34.f/255.f, 139.f/255.f, 34.f/255.f);
constexpr Color green(0.f/255.f, 128.f/255.f, 0.f/255.f);
constexpr Color dark_green(0.f/255.f, 100.f/255.f, 0.f/255.f);
constexpr Color yellow_green(154.f/255.f, 205.f/255.f, 50.f/255.f);
constexpr Color olive_drab(107.f/255.f, 142.f/255.f, 35.f/255.f);
constexpr Color olive(128.f/255.f, 128.f/255.f, 0.f/255.f);
constexpr Color dark_olive_green(85.f/255.f, 107.f/255.f, 47.f/255.f);
constexpr Color medium_aquamarine(102.f/255.f, 205.f/255.f, 170.f/255.f);
constexpr Color dark_sea_green(143.f/255.f, 188.f/255.f, 139.f/255.f);
constexpr Color light_sea_green(32.f/255.f, 178.f/255.f, 170.f/255.f);
constexpr Color dark_cyan(0.f/255.f, 139.f/255.f, 139.f/255.f);
constexpr Color teal(0.f/255.f, 128.f/255.f, 128.f/255.f);
constexpr Color aqua(0.f/255.f, 255.f/255.f, 255.f/255.f);
constexpr Color cyan(0.f/255.f, 255.f/255.f, 255.f/255.f);
constexpr Color light_cyan(224.f/255.f, 255.f/255.f, 255.f/255.f);
constexpr Color pale_turquoise(175.f/255.f, 238.f/255.f, 238.f/255.f);
constexpr Color aquamarine(127.f/255.f, 255.f/255.f, 212.f/255.f);
constexpr Color turquoise(64.f/255.f, 224.f/255.f, 208.f/255.f);
constexpr Color medium_turquoise(72.f/255.f, 209.f/255.f, 204.f/255.f);
constexpr Color dark_turquoise(0.f/255.f, 206.f/255.f, 209.f/255.f);
constexpr Color cadet_blue(95.f/255.f, 158.f/255.f, 160.f/255.f);
constexpr Color steel_blue(70.f/255.f, 130.f/255.f, 180.f/255.f);
constexpr Color light_steel_blue(176.f/255.f, 196.f/255.f, 222.f/255.f);
constexpr Color powder_blue(176.f/255.f, 224.f/255.f, 230.f/255.f);
constexpr Color light_blue(173.f/255.f, 216.f/255.f, 230.f/255.f);
constexpr Color sky_blue(135.f/255.f, 206.f/255.f, 235.f/255.f);
constexpr Color light_sky_blue(135.f/255.f, 206.f/255.f, 250.f/255.f);
constexpr Color deep_sky_blue(0.f/255.f, 191.f/255.f, 255.f/255.f);
constexpr Color dodger_blue(30.f/255.f, 144.f/255.f, 255.f/255.f);
constexpr Color cornflower_blue(100.f/255.f, 149.f/255.f, 237.f/255.f);
constexpr Color royal_blue(65.f/255.f, 105.f/255.f, 225.f/255.f);
constexpr Color blue(0.f/255.f, 0.f/255.f, 255.f/255.f);
constexpr Color medium_blue(0.f/255.f, 0.f/255.f, 205.f/255.f);
constexpr Color dark_blue(0.f/255.f, 0.f/255.f, 139.f/255.f);
constexpr Color navy(0.f/255.f, 0.f/255.f, 128.f/255.f);
constexpr Color midnight_blue(25.f/255.f, 25.f/255.f, 112.f/255.f);
constexpr Color cornsilk(255.f/255.f, 248.f/255.f, 220.f/255.f);
constexpr Color blanched_almond(255.f/255.f, 235.f/255.f, 205.f/255.f);
constexpr Color bisque(255.f/255.f, 228.f/255.f, 196.f/255.f);
constexpr Color navajo_white(255.f/255.f, 222.f/255.f, 173.f/255.f);
constexpr Color wheat(245.f/255.f, 222.f/255.f, 179.f/255.f);
constexpr Color burly_wood(222.f/255.f, 184.f/255.f, 135.f/255.f);
constexpr Color tan(210.f/255.f, 180.f/255.f, 140.f/255.f);
constexpr Color rosy_brown(188.f/255.f, 143.f/255.f, 143.f/255.f);
constexpr Color sandy_brown(244.f/255.f, 164.f/255.f, 96.f/255.f);
constexpr Color goldenrod(218.f/255.f, 165.f/255.f, 32.f/255.f);
constexpr Color dark_goldenrod(184.f/255.f, 134.f/255.f, 11.f/255.f);
constexpr Color peru(205.f/255.f, 133.f/255.f, 63.f/255.f);
constexpr Color chocolate(210.f/255.f, 105.f/255.f, 30.f/255.f);
constexpr Color saddle_brown(139.f/255.f, 69.f/255.f, 19.f/255.f);
constexpr Color sienna(160.f/255.f, 82.f/255.f, 45.f/255.f);
constexpr Color brown(165.f/255.f, 42.f/255.f, 42.f/255.f);
constexpr Color maroon(128.f/255.f, 0.f/255.f, 0.f/255.f);
constexpr Color white(255.f/255.f, 255.f/255.f, 255.f/255.f);
constexpr Color snow(255.f/255.f, 250.f/255.f, 250.f/255.f);
constexpr Color honey_dew(240.f/255.f, 255.f/255.f, 240.f/255.f);
constexpr Color mint_cream(245.f/255.f, 255.f/255.f, 250.f/255.f);
constexpr Color azure(240.f/255.f, 255.f/255.f, 255.f/255.f);
constexpr Color alice_blue(240.f/255.f, 248.f/255.f, 255.f/255.f);
constexpr Color ghost_white(248.f/255.f, 248.f/255.f, 255.f/255.f);
constexpr Color white_smoke(245.f/255.f, 245.f/255.f, 245.f/255.f);
constexpr Color sea_shell(255.f/255.f, 245.f/255.f, 238.f/255.f);
constexpr Color beige(245.f/255.f, 245.f/255.f, 220.f/255.f);
constexpr Color old_lace(253.f/255.f, 245.f/255.f, 230.f/255.f);
constexpr Color floral_white(255.f/255.f, 250.f/255.f, 240.f/255.f);
constexpr Color ivory(255.f/255.f, 255.f/255.f, 240.f/255.f);
constexpr Color antique_white(250.f/255.f, 235.f/255.f, 215.f/255.f);
constexpr Color linen(250.f/255.f, 240.f/255.f, 230.f/255.f);
constexpr Color lavender_blush(255.f/255.f, 240.f/255.f, 245.f/255.f);
constexpr Color misty_rose(255.f/255.f, 228.f/255.f, 225.f/255.f);
constexpr Color gainsboro(220.f/255.f, 220.f/255.f, 220.f/255.f);
constexpr Color light_gray(211.f/255.f, 211.f/255.f, 211.f/255.f);
constexpr Color silver(192.f/255.f, 192.f/255.f, 192.f/255.f);
constexpr Color dark_gray(169.f/255.f, 169.f/255.f, 169.f/255.f);
constexpr Color gray(128.f/255.f, 128.f/255.f, 128.f/255.f);
constexpr Color dim_gray(105.f/255.f, 105.f/255.f, 105.f/255.f);
constexpr Color light_slate_gray(119.f/255.f, 136.f/255.f, 153.f/255.f);
constexpr Color slate_gray(112.f/255.f, 128.f/255.f, 144.f/255.f);
constexpr Color dark_slate_gray(47.f/255.f, 79.f/255.f, 79.f/255.f);
constexpr Color near_black(10.f/255.f, 10.f/255.f, 10.f/255.f);
constexpr Color black(0.f/255.f, 0.f/255.f, 0.f/255.f);


}

}