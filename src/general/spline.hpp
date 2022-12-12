#pragma once

#include "vector.hpp"
#include "math.hpp"
#include "geometry.hpp"
#include "logger.hpp"

namespace spellbook {
inline v3 bspline(float t, const vector<v3>& points, u32 degrees = 4) {
    float fi = t * points.size();
    s32 center = degrees % 2 == 0 ? math::floor(fi) : math::round(fi);
    s32 start = center - math::floor(degrees / 2);

    auto mid_buffer = vector<v3>(points.begin() + start, points.begin() + start + degrees);
    while (degrees > 0) {
        for (u32 i = 0; (i+1) < degrees; ++i) {
            mid_buffer[i] = math::mix(mid_buffer[i], mid_buffer[i + 1], t);
        }
        degrees--;
    }

    return mid_buffer[0];
}


inline v3 bspline4(float t, span<const v3> points) {
    assert_else(points.size() == 4) {
        return v3(0);
    }
    return ((-points[0] + points[2]) * 0.5f +
        ((points[0] - 2.f * points[1] + points[2]) * 0.5f + (-points[0] + 3.f * points[1] - 3.f * points[2] + points[3]) * (1.f / 6.f) * t)
        * t) * t +
        (points[0] + 4.f * points[1] + points[2]) * (1.f / 6.f);
}

inline v3 bspline_v2(float t, const vector<v3>& points) {
    t = math::clamp(t, 0.0f, 0.99999f);
    if (points.size() < 2)
        return {};
    if (points.size() == 2)
        return math::mix(points[0], points[1], t);
    
    float fi = t * points.size();
    s32 center = math::floor(fi);
    s32 start = center - 2;

    if (start < -1 || start + 3 > points.size())
        return {};

    // bottom edge case
    if (start < 0) {
        vector points_section = {points[0], points[0], points[1], points[2]};
        return bspline4(math::fract(fi), points_section);
    }

    // top edge case
    if (start + 3 == points.size()) {
        vector points_section = {points[start], points[start+1], points[start+2], points[start+2]};
        return bspline4(math::fract(fi), points_section);
    }

    // general case
    span points_section(points.data() + start, 4);
    return bspline4(math::fract(fi), points_section);
}

}