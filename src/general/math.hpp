#pragma once

#include <cmath>

#include "geometry.hpp"

namespace spellbook::math {

const f32 PI  = 3.14159265359f;
const f32 TAU = 6.28318530718f;
const f32 D2R = PI / 180.0f;
const f32 R2D = 180.0f / PI;

f32   r2d(f32 rad);
f32   d2r(f32 deg);
euler r2d(euler rad);
euler d2r(euler deg);

bool contains(range r, f32 f);
bool contains(range2 r, v2 v);
bool contains(range2i r, v2i v);
bool contains(range3 r, v3 v);

s32 round_cast(float value);
v2i round_cast(v2 value);
v3i round_cast(v3 value);

s32 floor_cast(float value);
v2i floor_cast(v2 value);
v3i floor_cast(v3 value);

s32 ceil_cast(float value);
v2i ceil_cast(v2 value);
v3i ceil_cast(v3 value);

v3 convert_h(v4 v);
v4 convert_h(v3 v);

f32 length_squared(v2 v);
f32 length_squared(v3 v);
f32 length_squared_h(v4 v);
f32 length_squared(line2 v);
f32 length_squared(line3 v);

f32 length(v2 v);
f32 length(v3 v);
f32 length_h(v4 v);
f32 length(line2 v);
f32 length(line3 v);

f32 distance(v2 a, v2 b);
f32 distance(v3 a, v3 b);

f32 angle(v2 a);
f32 angle_difference(v2 a, v2 b);
f32 angle_to(v2 a, v2 b);

f32 dot(v2 a, v2 b);
f32 dot(v3 a, v3 b);

f32 cross(v2 a, v2 b);
v3  cross(v3 a, v3 b);
v3  ncross(v3 a, v3 b);

// transforms 0-1 to value in range
f32 to_range(f32 value, range r);
// transforms valaue in range to 0-1
f32 from_range(f32 value, range r);

bool iterate(v3i& current, const v3i& min, const v3i& max);

f32 cos(f32 angle);
f32 sin(f32 angle);
f32 tan(f32 angle);
f32 acos(f32 angle);
f32 asin(f32 angle);
f32 atan(f32 angle);
f32 atan2(f32 y, f32 x);
f32 atan3(f32 y, f32 x);

void random_seed(u32 seed);

s32 random_s32();
s32 random_s32(s32 high);
s32 random_s32(s32 low, s32 high);

f32 random_f32();
f32 random_f32(f32 high);
f32 random_f32(f32 low, f32 high);

u64 random_u64();

v3    euler2vector(euler e);
euler vector2euler(v3 v);

s32 ffsb(s32 input);
s32 csb(u8 input);
s32 csb(u32 input);
s32 csb(s32 input);

v2 project_point_onto_line(v2 point, line2 line);
v3 project_point_onto_plane(v3 point, v3 plane_point, v3 plane_normal);
v3 intersect_axis_plane(ray3 ray, u32 axis, f32 axis_value);

bool ray_intersects_aabb(v3 rstart, v3 rdir, v3 bstart, v3 bend, float* out_dist = nullptr);
bool ray_intersects_aabb(v2 rstart, v2 rdir, v2 bstart, v2 bend, float* out_dist = nullptr);

template <typename T> bool is_nan(T value) {
    return std::isnan(value);
}
template <> bool is_nan(v2 value);
template <> bool is_nan(v3 value);

template <typename T> bool is_inf(T value) {
    return std::isinf(value);
}
template <typename T> T abs(T value) {
    return std::abs(value);
}
template <typename T> constexpr T round(T value) {
    return std::round(value);
}
template <typename T> T floor(T value) {
    return std::floor(value);
}
template <typename T> T ceil(T value) {
    return std::ceil(value);
}
template <typename T> T ln(T value) {
    return std::log(value);
}
template <typename T> T log10(T value) {
    return std::log10(value);
}
template <typename T> T sqrt(T value) {
    return std::sqrt(value);
}
template <typename T> T cbrt(T value) {
    return std::cbrt(value);
}
template <typename T> T exp(T value) {
    return std::exp(value);
}
template <typename T> T hypot(T x, T y) {
    return std::hypot(x, y);
}
template <typename T> T pow(T base, T exp) {
    return std::pow(base, exp);
}
template <typename T> T copy_sign(T value, T sign) {
    return std::copysign(value, sign);
}

template <typename T> v2_<T> abs(v2_<T> v) {
    return v2_<T>(math::abs(v.x), math::abs(v.y));
}
template <typename T> constexpr v3_<T> abs(v3_<T> v) {
    return v3_<T>(math::abs(v.x), math::abs(v.y), math::abs(v.z));
}
template <typename T> v4_<T> abs(v4_<T> v) {
    return v4_<T>(math::abs(v.x), math::abs(v.y), math::abs(v.z), math::abs(v.w));
}

template <typename T> v2_<T> round(v2_<T> v) {
    return v2_<T>(math::round(v.x), math::round(v.y));
}
template <typename T> v3_<T> round(v3_<T> v) {
    return v3_<T>(math::round(v.x), math::round(v.y), math::round(v.z));
}
template <typename T> v4_<T> round(v4_<T> v) {
    return v4_<T>(math::round(v.x), math::round(v.y), math::round(v.z), math::round(v.w));
}

template <typename T> v2_<T> floor(v2_<T> v) {
    return v2_<T>(math::floor(v.x), math::floor(v.y));
}
template <typename T> v3_<T> floor(v3_<T> v) {
    return v3_<T>(math::floor(v.x), math::floor(v.y), math::floor(v.z));
}
template <typename T> v4_<T> floor(v4_<T> v) {
    return v4_<T>(math::floor(v.x), math::floor(v.y), math::floor(v.z), math::floor(v.w));
}

template <typename T> v2_<T> pow(v2_<T> base, T exp) {
    return v2_<T>(math::pow(base.x, exp), math::pow(base.y, exp));
}
template <typename T> v3_<T> pow(v3_<T> base, T exp) {
    return v3_<T>(math::pow(base.x, exp), math::pow(base.y, exp), math::pow(base.z, exp));
}
template <typename T> v4_<T> pow(v4_<T> base, T exp) {
    return v4_<T>(math::pow(base.x, exp), math::pow(base.y, exp), math::pow(base.z, exp), math::pow(base.w, exp));
}

template <typename T> v2_<T> copy_sign(v2_<T> value, v2_<T> sign) {
    return v2_<T>(math::copy_sign(value.x, sign.x), math::copy_sign(value.y, sign.y));
}
template <typename T> v3_<T> copy_sign(v3_<T> value, v3_<T> sign) {
    return v3_<T>(math::copy_sign(value.x, sign.x), math::copy_sign(value.y, sign.y), math::copy_sign(value.z, sign.z));
}
template <typename T> v4_<T> copy_sign(v4_<T> value, v4_<T> sign) {
    return v4_<T>(
        math::copy_sign(value.x, sign.x), math::copy_sign(value.y, sign.y), math::copy_sign(value.z, sign.z), math::copy_sign(value.w, sign.w));
}

template <typename T> constexpr const T& clamp(const T& value, const T& minValue, const T& maxValue) {
    if (value < minValue)
        return minValue;
    else if (value > maxValue)
        return maxValue;
    else
        return value;
}

template <typename T> constexpr v2_<T> clamp(v2_<T> value, v2_<T> low, v2_<T> high) {
    return v2_<T>(math::clamp(value.x, low.x, high.x), math::clamp(value.y, low.y, high.y));
}
template <typename T> constexpr v3_<T> clamp(v3_<T> value, v3_<T> low, v3_<T> high) {
    return v3_<T>(math::clamp(value.x, low.x, high.x), math::clamp(value.y, low.y, high.y), math::clamp(value.z, low.z, high.z));
}

template <typename T> T max(T a, T b) {
    return (a > b) ? a : b;
}
template <typename T, typename... Ts> T max(T a, T b, Ts... c) {
    return math::max(math::max(a, b), c...);
}
template <typename T> v2_<T> max(v2_<T> a, v2_<T> b) {
    return v2_<T>(math::max(a.x, b.x), math::max(a.y, b.y));
}
template <typename T> v3_<T> max(v3_<T> a, v3_<T> b) {
    return v3_<T>(math::max(a.x, b.x), math::max(a.y, b.y), math::max(a.z, b.z));
}

template <typename T> T min(T a, T b) {
    return (a < b) ? a : b;
}
template <typename T, typename... Ts> T min(T a, T b, Ts... c) {
    return math::min(math::min(a, b), c...);
}
template <typename T> v2_<T> min(v2_<T> a, v2_<T> b) {
    return v2_<T>(math::min(a.x, b.x), math::min(a.y, b.y));
}
template <typename T> v3_<T> min(v3_<T> a, v3_<T> b) {
    return v3_<T>(math::min(a.x, b.x), math::min(a.y, b.y), math::min(a.z, b.z));
}

template <typename T> bool is_equal(T a, T b, T e) {
    return math::abs(a - b) < e;
}
template <typename T> bool is_equal(v2_<T> a, v2_<T> b, T e) {
    return math::is_equal(a.x, b.x, e) && math::is_equal(a.y, b.y, e);
}
template <typename T> bool is_equal(v3_<T> a, v3_<T> b, T e) {
    return math::is_equal(a.x, b.x, e) && math::is_equal(a.y, b.y, e) && math::is_equal(a.z, b.z, e);
}
template <typename T> bool is_equal(v4_<T> a, v4_<T> b, T e) {
    return math::is_equal(a.x, b.x, e) && math::is_equal(a.y, b.y, e) && math::is_equal(a.z, b.z, e) && math::is_equal(a.w, b.w, e);
}

template <typename T> T normalize(T t) {
    return t / math::length(t);
}

template <typename T> T distance_to_segment(v2_<T> pos32, line_<v2_<T>> line) {
    v2_<T> s2p = pos32 - line.start;
    v2_<T> s2e = line.end - line.start;
    s2e        = s2e / math::length(s2e);

    f32 d = dot(s2p, s2e);
    if (d < 0 || 1 < d)
        return -1;
    return length(cross(s2p, s2e));
}

// Converts 0to1 values to -1to1
template <typename T> T to_signed_range(T value, bool invert = false) {
    return ((invert ? (1.0f - value) : value) * 2.0f) - 1.0f;
}

// Converts -1to1 values to 0to1
template <typename T> T to_unsigned_range(T value, bool invert = false) {
    return ((invert ? -1.0f : 1.0f) * value + 1.0f) / 2.0f;
}

template <typename T> bool intersect_of_line_segments(const v2_<T> p0, const v2_<T> p1, const v2_<T> q0, const v2_<T> q1, v2_<T>* result) {
    v2_<T> v = p1 - p0;
    v2_<T> u = q1 - q0;
    v2_<T> d = p0 - q0;
    T      c = math::cross(v, u);
    if (math::is_equal(c, 0.0f, 0.00001f))
        return false;
    T s = math::cross(v, d) / c;
    T t = math::cross(u, d) / c;
    if (s < 0 || s > 1 || t < 0 || t > 1)
        return false;
    *result = p0 + t * v;
    return true;
}

template <typename T> constexpr T mix(T a, T b, f32 x) {
    return a*(1.f-x)+b*x;
}

enum EaseMode {
    EaseMode_Linear,
    EaseMode_Quad,
    EaseMode_QuadOut,
    EaseMode_QuadIn,
    EaseMode_Cubic,
    EaseMode_CubicOut,
    EaseMode_CubicIn,
    EaseMode_ElasticOut,
    EaseMode_ElasticIn,
    EaseMode_Elastic
};
constexpr float ease(float x, EaseMode mode) {
    switch (mode) {
        case (EaseMode_Linear): {
            return x;
        } break;
        case (EaseMode_Quad): {
            return x < 0.5f ? 2.0f * math::pow(x, 2.0f) : 1.0f - math::pow(-2.0f * x + 2.0f, 2.0f) / 2.0f;
        } break;
        case (EaseMode_QuadOut): {
            return math::pow(x, 2.0f);
        } break;
        case (EaseMode_QuadIn): {
            return 1.0f - math::pow(1.0f - x, 2.0f);
        } break;
        case (EaseMode_Cubic): {
            return x < 0.5f ? 4.0f * math::pow(x, 3.0f) : 1.0f - math::pow(-2.0f * x + 2.0f, 3.0f) / 2.0f;
        } break;
        case (EaseMode_CubicOut): {
            return math::pow(x, 3.0f);
        } break;
        case (EaseMode_CubicIn): {
            return 1.0f - math::pow(1.0f - x, 3.0f);
        } break;
        case (EaseMode_Elastic): {
            const float c5 = (2.0f * math::PI) / 4.5f;
            return x == 0.0f ? 0.0f
                : x == 1.0f ? 1.0f
                    : x < 0.5f ? -(math::pow(2.0f, 20 * x - 10.0f) * math::sin((20.0f * x - 11.125f) * c5)) / 2.0f
                        : (math::pow(2.0f, -20.0f * x + 10.0f) * math::sin((20.0f * x - 11.125f) * c5)) / 2.0f + 1.0f;
        } break;
        default: {
            __debugbreak();
            return x;
        }
    }
}

inline f32 smoothstep(f32 edge0, f32 edge1, f32 x) {
    x = math::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
}

inline f32 mod(f32 input, f32 divisor) {
    return std::fmod(input, divisor);
}

inline s32 mod(s32 input, s32 divisor) {
    return input % divisor;
}

template <typename T>
v2_<T> mod(v2_<T> input, v2_<T> divisor) {
    return v2_<T>{math::mod(input.x, divisor.x), math::mod(input.y, divisor.y)};
}

template <typename T>
v3_<T> mod(v3_<T> input, v3_<T> divisor) {
    return v3_<T>{math::mod(input.x, divisor.x), math::mod(input.y, divisor.y), math::mod(input.z, divisor.z)};
}

inline f32 fract(f32 input) {
    return math::mod(input, 1.0f);
}

inline f32 lerp_angle(f32 t, range r) {
    f32 diff = r.end - r.start;

    diff = math::mod(diff + math::PI, math::TAU);
    if (diff < 0.0f)
        diff += math::TAU;
    diff -= math::PI;

    return r.start + (diff * t);
}

inline float angle_difference(float a, float b) {
    return atan2(sin(b-a), cos(b-a));
}

inline float wrap_angle (float angle) {
    return angle - math::TAU * floor( angle / math::TAU);
};

inline bool contains_angle(range r, float angle) {
    r.start = wrap_angle(r.start);
    r.end = wrap_angle(r.end);
    angle = wrap_angle(angle);

    if (r.start < r.end)
        return (angle > r.start && angle < r.end);
    if (r.start > r.end)
        return (angle > r.start || angle < r.end);
    return (angle == r.start);
}

inline float clamp_angle(float angle, range r) {
    r.start = wrap_angle(r.start);
    r.end = wrap_angle(r.end - 0.001f);
    angle = wrap_angle(angle);

    // Clamp the angle to the range of the two given angles
    if (r.start < r.end)
        return math::max(r.start, math::min(r.end, angle));
    if (r.start > r.end) {
        if (angle > r.start || angle < r.end)
            return angle;
        if (math::abs(r.end - angle) < math::abs(r.start - angle))
            return r.end;
        return r.start;
    }
    return r.start;
}

inline f32 clamp(f32 input, range range) {
    return math::clamp(input, range.start, range.end);
}

inline f32 map_range(f32 input, range input_range, range output_range) {
    float value = clamp(from_range(input, input_range), {0.0f, 1.0f});
    value = to_range(value, output_range);
    return value;
}

constexpr f32 lerp(f32 x, range r) {
    return r.start*(1.0f-x)+r.end*x;
}
constexpr v3 lerp(f32 x, range3 r) {
    return r.start*(1.0f-x)+r.end*x;
}

#define d_(a,b,c,d) (a.x-b.x)*(c.x-d.x)+(a.y-b.y)*(c.y-d.y)+(a.z-b.z)*(c.z-d.z);
inline f32 line_intersection_3d(const ray3& a, const ray3& b) {
    auto o1 = a.origin;
    auto e1 = a.origin + a.dir;
    auto o2 = b.origin;
    auto e2 = b.origin + b.dir;
    f32 d1321 = d_(o1, o2, e1, o1);
    f32 d2121 = d_(e1, o1, e1, o1);
    f32 d4321 = d_(e2, o2, e1, o1);
    f32 d1343 = d_(o1, o2, e2, o2);
    f32 d4343 = d_(e2, o2, e2, o2);
    return (d1343*d4321-d1321*d4343)/(d2121*d4343-d4321*d4321);
}

v3 rotate(v3 v, u32 cardinal_rotation);

}
