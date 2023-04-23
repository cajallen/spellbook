#include "color.hpp"

#include <imgui.h>
#include <vuk/Types.hpp>
#include "math.hpp"
#include "tracy/Tracy.hpp"

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

v3 srgb2linear(v3 v) {
    return v3(srgb2linear(v.x), srgb2linear(v.y), srgb2linear(v.z));
}

v3 linear2srgb(v3 v) {
    return v3(linear2srgb(v.x), linear2srgb(v.y), linear2srgb(v.z));
}

Color::operator vuk::ClearColor() const {
    return {srgb2linear(r), srgb2linear(g), srgb2linear(b), a};
}

float toe_inv(float x) {
    float k_1 = 0.206f;
    float k_2 = 0.03f;
    float k_3 = (1.f + k_1) / (1.f + k_2);
    return (x * x + k_1 * x) / (k_3 * (x + k_2));
}
v2 to_ST(v2 cusp)
{
    float L = cusp.x;
    float C = cusp.y;
    return v2( C / L, C / (1.f - L) );
}

float compute_max_saturation(float a, float b)
{
	// Max saturation will be when one of r, g or b goes below zero.

	// Select different coefficients depending on which component goes below zero first
	float k0, k1, k2, k3, k4, wl, wm, ws;

	if (-1.88170328f * a - 0.80936493f * b > 1.f)
	{
		// Red component
		k0 = +1.19086277f; k1 = +1.76576728f; k2 = +0.59662641f; k3 = +0.75515197f; k4 = +0.56771245f;
		wl = +4.0767416621f; wm = -3.3077115913f; ws = +0.2309699292f;
	}
	else if (1.81444104f * a - 1.19445276f * b > 1.f)
	{
		// Green component
		k0 = +0.73956515f; k1 = -0.45954404f; k2 = +0.08285427f; k3 = +0.12541070f; k4 = +0.14503204f;
		wl = -1.2684380046f; wm = +2.6097574011f; ws = -0.3413193965f;
	}
	else
	{
		// Blue component
		k0 = +1.35733652f; k1 = -0.00915799f; k2 = -1.15130210f; k3 = -0.50559606f; k4 = +0.00692167f;
		wl = -0.0041960863f; wm = -0.7034186147f; ws = +1.7076147010f;
	}

	// Approximate max saturation using a polynomial:
	float S = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

	// Do one step Halley's method to get closer
	// this gives an error less than 10e6, except for some blue hues where the dS/dh is close to infinite
	// this should be sufficient for most applications, otherwise do two/three steps 

	float k_l = +0.3963377774f * a + 0.2158037573f * b;
	float k_m = -0.1055613458f * a - 0.0638541728f * b;
	float k_s = -0.0894841775f * a - 1.2914855480f * b;

	{
		float l_ = 1.f + S * k_l;
		float m_ = 1.f + S * k_m;
		float s_ = 1.f + S * k_s;

		float l = l_ * l_ * l_;
		float m = m_ * m_ * m_;
		float s = s_ * s_ * s_;

		float l_dS = 3.f * k_l * l_ * l_;
		float m_dS = 3.f * k_m * m_ * m_;
		float s_dS = 3.f * k_s * s_ * s_;

		float l_dS2 = 6.f * k_l * k_l * l_;
		float m_dS2 = 6.f * k_m * k_m * m_;
		float s_dS2 = 6.f * k_s * k_s * s_;

		float f = wl * l + wm * m + ws * s;
		float f1 = wl * l_dS + wm * m_dS + ws * s_dS;
		float f2 = wl * l_dS2 + wm * m_dS2 + ws * s_dS2;

		S = S - f * f1 / (f1 * f1 - 0.5f * f * f2);
	}

	return S;
}


v3 oklab_to_linear_srgb(v3 c)
{
    float l_ = c.x + 0.3963377774f * c.y + 0.2158037573f * c.z;
    float m_ = c.x - 0.1055613458f * c.y - 0.0638541728f * c.z;
    float s_ = c.x - 0.0894841775f * c.y - 1.2914855480f * c.z;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return v3(
        +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
        -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
        -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s
    );
}


v2 find_cusp(float a, float b)
{
    // First, find the maximum saturation (saturation S = C/L)
    float S_cusp = compute_max_saturation(a, b);

    // Convert to linear sRGB to find the first point where at least one of r,g or b >= 1:
    v3 rgb_at_max = oklab_to_linear_srgb(v3( 1, S_cusp * a, S_cusp * b ));
    float L_cusp = cbrt(1.f / math::max(math::max(rgb_at_max.r, rgb_at_max.g), rgb_at_max.b));
    float C_cusp = L_cusp * S_cusp;

    return v2( L_cusp , C_cusp );
}

float find_gamut_intersection(float a, float b, float L1, float C1, float L0, v2 cusp)
{
	// Find the intersection for upper and lower half seprately
	float t;
	if (((L1 - L0) * cusp.y - (cusp.x - L0) * C1) <= 0.f)
	{
		// Lower half

		t = cusp.y * L0 / (C1 * cusp.x + cusp.y * (L0 - L1));
	}
	else
	{
		// Upper half

		// First intersect with triangle
		t = cusp.y * (L0 - 1.f) / (C1 * (cusp.x - 1.f) + cusp.y * (L0 - L1));

		// Then one step Halley's method
		{
			float dL = L1 - L0;
			float dC = C1;

			float k_l = +0.3963377774f * a + 0.2158037573f * b;
			float k_m = -0.1055613458f * a - 0.0638541728f * b;
			float k_s = -0.0894841775f * a - 1.2914855480f * b;

			float l_dt = dL + dC * k_l;
			float m_dt = dL + dC * k_m;
			float s_dt = dL + dC * k_s;


			// If higher accuracy is required, 2 or 3 iterations of the following block can be used:
			{
				float L = L0 * (1.f - t) + t * L1;
				float C = t * C1;

				float l_ = L + C * k_l;
				float m_ = L + C * k_m;
				float s_ = L + C * k_s;

				float l = l_ * l_ * l_;
				float m = m_ * m_ * m_;
				float s = s_ * s_ * s_;

				float ldt = 3.f * l_dt * l_ * l_;
				float mdt = 3.f * m_dt * m_ * m_;
				float sdt = 3.f * s_dt * s_ * s_;

				float ldt2 = 6.f * l_dt * l_dt * l_;
				float mdt2 = 6.f * m_dt * m_dt * m_;
				float sdt2 = 6.f * s_dt * s_dt * s_;

				float r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s - 1.f;
				float r1 = 4.0767416621f * ldt - 3.3077115913f * mdt + 0.2309699292f * sdt;
				float r2 = 4.0767416621f * ldt2 - 3.3077115913f * mdt2 + 0.2309699292f * sdt2;

				float u_r = r1 / (r1 * r1 - 0.5f * r * r2);
				float t_r = -r * u_r;

				float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s - 1.f;
				float g1 = -1.2684380046f * ldt + 2.6097574011f * mdt - 0.3413193965f * sdt;
				float g2 = -1.2684380046f * ldt2 + 2.6097574011f * mdt2 - 0.3413193965f * sdt2;

				float u_g = g1 / (g1 * g1 - 0.5f * g * g2);
				float t_g = -g * u_g;

				float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s - 1.f;
				float b1 = -0.0041960863f * ldt - 0.7034186147f * mdt + 1.7076147010f * sdt;
				float b2 = -0.0041960863f * ldt2 - 0.7034186147f * mdt2 + 1.7076147010f * sdt2;

				float u_b = b1 / (b1 * b1 - 0.5f * b * b2);
				float t_b = -b * u_b;

				t_r = u_r >= 0.f ? t_r : 10000.f;
				t_g = u_g >= 0.f ? t_g : 10000.f;
				t_b = u_b >= 0.f ? t_b : 10000.f;

				t += math::min(t_r, math::min(t_g, t_b));
			}
		}
	}

	return t;
}


float find_gamut_intersection(float a, float b, float L1, float C1, float L0)
{
    // Find the cusp of the gamut triangle
    v2 cusp = find_cusp(a, b);

    return find_gamut_intersection(a, b, L1, C1, L0, cusp);
}

v2 get_ST_mid(float a_, float b_)
{
    float S = 0.11516993f + 1.f / (
        +7.44778970f + 4.15901240f * b_
        + a_ * (-2.19557347f + 1.75198401f * b_
            + a_ * (-2.13704948f - 10.02301043f * b_
                + a_ * (-4.24894561f + 5.38770819f * b_ + 4.69891013f * a_
                    )))
        );

    float T = 0.11239642f + 1.f / (
        +1.61320320f - 0.68124379f * b_
        + a_ * (+0.40370612f + 0.90148123f * b_
            + a_ * (-0.27087943f + 0.61223990f * b_
                + a_ * (+0.00299215f - 0.45399568f * b_ - 0.14661872f * a_
                    )))
        );

    return v2(S, T);
}

v3 get_Cs(float L, float a_, float b_)
{
    v2 cusp = find_cusp(a_, b_);

    float C_max = find_gamut_intersection(a_, b_, L, 1.f, L, cusp);
    v2 ST_max = to_ST(cusp);
	
    // Scale factor to compensate for the curved part of gamut shape:
    float k = C_max / math::min((L * ST_max.x), (1.f - L) * ST_max.y);

    float C_mid;
    {
        v2 ST_mid = get_ST_mid(a_, b_);

        // Use a soft minimum function, instead of a sharp triangle shape to get a smooth value for chroma.
        float C_a = L * ST_mid.x;
        float C_b = (1.f - L) * ST_mid.y;
        C_mid = 0.9f * k * sqrt(sqrt(1.f / (1.f / (C_a * C_a * C_a * C_a) + 1.f / (C_b * C_b * C_b * C_b))));
    }

    float C_0;
    {
        // for C_0, the shape is independent of hue, so v2 are constant. Values picked to roughly be the average values of v2.
        float C_a = L * 0.4f;
        float C_b = (1.f - L) * 0.8f;

        // Use a soft minimum function, instead of a sharp triangle shape to get a smooth value for chroma.
        C_0 = sqrt(1.f / (1.f / (C_a * C_a) + 1.f / (C_b * C_b)));
    }

    return v3( C_0, C_mid, C_max );
}

float srgb_transfer_function(float a)
{
    return .0031308f >= a ? 12.92f * a : 1.055f * pow(a, .4166666666666667f) - .055f;
}

Color Color::hsl_oklab(float hue, float sat, float lum) {
        if (lum == 1.0f)
            return Color(1.f, 1.f, 1.f);

        if (lum == 0.f)
            return Color(1.f, 1.f, 1.f);

        float a_ = cos(math::TAU * hue);
        float b_ = sin(math::TAU * hue);
        float L = toe_inv(lum);

        v3 cs = get_Cs(L, a_, b_);
        float C_0 = cs.x;
        float C_mid = cs.y;
        float C_max = cs.z;

        float mid = 0.8f;
        float mid_inv = 1.25f;

        float C, t, k_0, k_1, k_2;

        if (sat < mid)
        {
            t = mid_inv * sat;

            k_1 = mid * C_0;
            k_2 = (1.f - k_1 / C_mid);

            C = t * k_1 / (1.f - k_2 * t);
        }
        else
        {
            t = (sat - mid)/ (1.f - mid);

            k_0 = C_mid;
            k_1 = (1.f - mid) * C_mid * C_mid * mid_inv * mid_inv / C_0;
            k_2 = (1.f - (k_1) / (C_max - C_mid));

            C = k_0 + t * k_1 / (1.f - k_2 * t);
        }

        v3 rgb = oklab_to_linear_srgb(v3( L, C * a_, C * b_ ));
        return Color(
            srgb_transfer_function(rgb.r),
            srgb_transfer_function(rgb.g),
            srgb_transfer_function(rgb.b),
            1.0f
        );
}

#define COLOR_VIEW(var) { \
    f32 var##copy[4] = { var.r, var.g, var.b, var.a }; \
    ImGui::ColorEdit4(#var, var##copy, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_NoInputs); \
}

using namespace palette;
void color_window(bool* p_open) {
    ZoneScoped;
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
