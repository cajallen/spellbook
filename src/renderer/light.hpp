#pragma once

#include "general/color.hpp"
#include "general/json.hpp"
#include "general/math/geometry.hpp"
#include "general/math/math.hpp"

namespace spellbook {

struct PointLight {
    v3 position = {};
    euler dir = {};
    Color color = palette::white;
};

struct DirectionalLight {
    euler dir = {};
    Color color = palette::white;
};

struct LightGPU {
    union {
        v4 position;
        v3 vector;
    };
    v4 color;

    LightGPU(const DirectionalLight& light) {
        vector = math::euler2vector(light.dir);
        color = v4(light.color.rgb * light.color.a, 0.0f);
    }
};



JSON_IMPL(DirectionalLight, dir, color);

void inspect(DirectionalLight* light);

}
