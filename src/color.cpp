#include "color.hpp"

#include <imgui.h>
#include <vuk/Types.hpp>
#include "math.hpp"

namespace spellbook {

float srgb2linear(float x) {
    if (x <= 0.0f)
        return 0.0f;
    else if (x >= 1.0f)
        return 1.0f;
    else if (x < 0.04045f)
        return x / 12.92f;
    else
        return std::pow((x + 0.055f) / 1.055f, 2.4f);
}

float linear2srgb(float x) {
    if (x <= 0.0f)
        return 0.0f;
    else if (x >= 1.0f)
        return 1.0f;
    else if (x < 0.0031308f)
        return x * 12.92f;
    else
        return std::pow(x, 1.0f / 2.4f) * 1.055f - 0.055f;
}

Color::operator vuk::ClearColor() const {
    return {srgb2linear(r), srgb2linear(g), srgb2linear(b), a};
}

}
