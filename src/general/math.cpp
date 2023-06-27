#include "math.hpp"

#include "logger.hpp"

#include <cmath>
#include <random>

namespace spellbook::math {

f32 r2d(f32 rad) {
    return rad * (180.0f / PI);
}

f32 d2r(f32 deg) {
    return deg * (PI / 180.0f);
}

euler r2d(euler rad) {
    return euler{math::r2d(rad.yaw), math::r2d(rad.pitch), math::r2d(rad.roll)};
}

euler d2r(euler deg) {
    return euler{math::d2r(deg.yaw), math::d2r(deg.pitch), math::d2r(deg.roll)};
}

bool contains(range r, f32 f) {
    return r.start < f && f < r.end;
}

bool contains(range2 r, v2 v) {
    return r.start[0] < v[0] && v[0] < r.end[0] && r.start[1] < v[1] && v[1] < r.end[1];
}

bool contains(range2i r, v2i v) {
    return r.start[0] <= v[0] && v[0] < r.end[0] && r.start[1] <= v[1] && v[1] < r.end[1];
}

bool contains(range3 r, v3 v) {
    return r.start[0] < v[0] && v[0] < r.end[0] && r.start[1] < v[1] && v[1] < r.end[1] && r.start[2] < v[2] && v[2] < r.end[2];
}

s32 round_cast(float value) {
    return (s32) math::round(value);
}
v2i round_cast(v2 value) {
    return v2i((s32) math::round(value.x), (s32) math::round(value.y));
}
v3i round_cast(v3 value) {
    return v3i((s32) math::round(value.x), (s32) math::round(value.y), (s32) math::round(value.z));
}

s32 floor_cast(float value) {
    return (s32) math::floor(value);
}
v2i floor_cast(v2 value) {
    return v2i((s32) math::floor(value.x), (s32) math::floor(value.y));
}
v3i floor_cast(v3 value) {
    return v3i((s32) math::floor(value.x), (s32) math::floor(value.y), (s32) math::floor(value.z));
}

s32 ceil_cast(float value) {
    return (s32) math::ceil(value);
}
v2i ceil_cast(v2 value) {
    return v2i((s32) math::ceil(value.x), (s32) math::ceil(value.y));
}
v3i ceil_cast(v3 value) {
    return v3i((s32) math::ceil(value.x), (s32) math::ceil(value.y), (s32) math::ceil(value.z));
}

v3 convert_h(v4 v) {
    return v3 {v.x / v.w, v.y / v.w, v.z / v.w};
}

v4 convert_h(v3 v) {
    return v4 {v.x, v.y, v.z, 1.0f};
}

f32 length_squared(v2 v) {
    return v.x * v.x + v.y * v.y;
}

f32 length_squared(v3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

f32 length_squared_h(v4 v) {
    return length(convert_h(v));
}

f32 length(f32 v) {
    return abs(v);
}

f32 length(v2 v) {
    return sqrt(length_squared(v));
}

f32 length(v3 v) {
    return sqrt(length_squared(v));
}

f32 length_h(v4 v) {
    return sqrt(length_squared_h(v));
}

f32 angle(v2 a) {
    return atan2(a.y, a.x);
}

f32 angle_difference(v2 a, v2 b) {
    return angle(b) - angle(a);
}

f32 angle_to(v2 a, v2 b) {
    return angle(b - a);
}

f32 dot(v2 a, v2 b) {
    return a.x * b.x + a.y * b.y;
}

f32 dot(v3 a, v3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

f32 dot(v4 a, v4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

f32 cross(v2 a, v2 b) {
    return a.x * b.y - a.y * b.x;
}

v3 cross(v3 a, v3 b) {
    v3 out;
    out.x = a.y * b.z - a.z * b.y;
    out.y = a.z * b.x - a.x * b.z;
    out.z = a.x * b.y - a.y * b.x;
    return out;
}

v3 ncross(v3 a, v3 b) {
    return normalize(cross(a, b));
}

f32 to_range(f32 value, range r) {
    return r.start + value * (r.end - r.start);
}

f32 from_range(f32 value, range r) {
    if (r.end - r.start == 0.0f)
        return 0.0f;
    return (value - r.start) / (r.end - r.start);
}

bool iterate(v3i& current, const v3i& min, const v3i& max) {
    current.x += 1;
    if (current.x > max.x) {
        current.y += 1;
        current.x = min.x;
    }
    if (current.y > max.y) {
        current.z += 1;
        current.y = min.y;
    }
    if (current.z > max.z)
        return false;
    return true;
}

f32 length_squared(line2 v) {
    return math::length_squared(v.end - v.start);
}

f32 length_squared(line3 v) {
    return math::length_squared(v.end - v.start);
}

f32 length(line2 v) {
    return math::length(v.end - v.start);
}

f32 length(line3 v) {
    return math::length(v.end - v.start);
}

f32 distance(v2 a, v2 b) {
    return math::length(b - a);
}

f32 distance(v3 a, v3 b) {
    return math::length(b - a);
}

void random_seed(u32 seed) {
    srand(seed);
}

s32 random_s32() {
    return rand();
}
s32 random_s32(s32 high) {
    return math::random_s32() % high;
}
s32 random_s32(s32 low, s32 high) {
    return low + math::random_s32(high - low + 1);
}

f32 random_f32() {
    return rand() / (f32) RAND_MAX;
}
f32 random_f32(f32 high) {
    return math::random_f32() * high;
}
f32 random_f32(f32 low, f32 high) {
    return low + math::random_f32(high - low);
}

u64 random_u64() {
    static std::random_device rd;
    static std::mt19937_64  random_engine(rd());
    static std::uniform_int_distribution<u64> uniform_dist;
    return uniform_dist(random_engine);
}

v2 project_point_onto_line(v2 point, line2 line) {
    return line.end * (math::dot(line.vector(), point - line.start) / math::length_squared(line));
}

v3 project_point_onto_line(v3 point, line3 line) {
    return line.end * (math::dot(line.vector(), point - line.start) / math::length_squared(line));
}

v3 project_to_segment(v3 point, line3 line) {
    v3 line_vec = line.vector();
    float len_sq = math::length_squared(line_vec);
    if (len_sq == 0.0f)
        return line.start;
    float d = math::dot(line_vec, point - line.start);
    float t = math::clamp(d / len_sq, {0.0f, 1.0f});
    return line.start + t * line_vec;
}

v3 project_point_onto_plane(v3 point, v3 plane_point, v3 plane_normal) {
    return point - math::dot(point - plane_point, plane_normal) * plane_normal;
}

f32 cos(f32 angle) {
    return std::cos(angle);
}
f32 sin(f32 angle) {
    return std::sin(angle);
}
f32 tan(f32 angle) {
    return std::tan(angle);
}
f32 acos(f32 angle) {
    return std::acos(angle);
}
f32 asin(f32 angle) {
    return std::asin(angle);
}
f32 atan(f32 angle) {
    return std::atan(angle);
}
f32 atan2(f32 y, f32 x) {
    return std::atan2(y, x);
}
f32 atan3(f32 y, f32 x) {
    return y < 0 ? std::atan2(y, x) + math::TAU : std::atan2(y, x);
}

s32 ffsb(s32 input) {
    for (s32 i = 0; i < sizeof(s32) * 8; i++) {
        if (input & 0b1 << i)
            return i;
    }
    return -1;
}

s32 csb(u8 input) {
    return std::popcount(input);
}

s32 csb(u32 input) {
    return std::popcount(input);
}

s32 csb(s32 input) {
    s32 n = 0;
    for (s32 i = 0; i < sizeof(s32) * 8; i++) {
        if (input & 0b1 << i)
            n++;
    }
    return n;
}

v3 intersect_axis_plane(ray3 ray, u32 axis, f32 axis_value) {
    f64 t     = (axis_value - ray.origin[axis]) / ray.dir[axis];
    u32 axis1 = (axis + 1) % 3;
    u32 axis2 = (axis + 2) % 3;
    v3 ret = {};
    ret[axis] = axis_value;
    ret[axis1] = ray.origin[axis1] + ray.dir[axis1] * t;
    ret[axis2] = ray.origin[axis2] + ray.dir[axis2] * t;
    return ret;
}

v3 euler2vector(euler e) {
    return math::normalize(v3 {math::cos(e.pitch) * math::cos(e.yaw), math::cos(e.pitch) * math::sin(e.yaw), math::sin(e.pitch)});
}

euler vector2euler(v3 v) {
    v         = math::normalize(v);
    f32 pitch = math::asin(v.z);
    f32 yaw   = math::atan2(v.y, v.x);
    return euler{yaw, pitch};
}

bool ray_intersects_aabb(v3 rstart, v3 rdir, v3 bstart, v3 bend, float* out_dist) {
    float tx1 = (bstart.x - rstart.x) / rdir.x;
    float tx2 = (bend.x - rstart.x) / rdir.x;

    float tmin = math::min(tx1, tx2);
    float tmax = math::max(tx1, tx2);

    float ty1 = (bstart.y - rstart.y) / rdir.y;
    float ty2 = (bend.y - rstart.y) / rdir.y;

    tmin = math::max(tmin, math::min(ty1, ty2));
    tmax = math::min(tmax, math::max(ty1, ty2));

    float tz1 = (bstart.z - rstart.z) / rdir.z;
    float tz2 = (bend.z - rstart.z) / rdir.z;

    tmin = math::max(tmin, math::min(tz1, tz2));
    tmax = math::min(tmax, math::max(tz1, tz2));

    if (out_dist != nullptr)
        *out_dist = math::max(0.0f, tmin);
    return tmax >= math::max(0.0f, tmin) && FLT_MAX > tmin;
}

// https://www.desmos.com/calculator/rryes1oucs
// Not fully sure why the graph isn't completely accurate, but the algorithm seems to work
// so there is probably a small error in the graph. - Cole
bool ray_intersects_aabb(v2 rstart, v2 rdir, v2 bstart, v2 bend, float* out_dist) {
    float tx1 = (bstart.x - rstart.x) / rdir.x;
    float tx2 = (bend.x - rstart.x) / rdir.x;

    float tmin = math::min(tx1, tx2); // segments to first intercept
    float tmax = math::max(tx1, tx2); // segments to second intercept

    float ty1 = (bstart.y - rstart.y) / rdir.y; //
    float ty2 = (bend.y - rstart.y) / rdir.y;

    tmin = math::max(tmin, math::min(ty1, ty2));
    tmax = math::min(tmax, math::max(ty1, ty2));

    if (out_dist != nullptr)
        *out_dist = math::max(0.0f, tmin);
    return tmax >= math::max(0.0f, tmin) && tmin < FLT_MAX;
}

v3 rotate(v3 v, u32 cardinal_rotation) {
    if (cardinal_rotation % 4 == 0)
        return v;
    if (cardinal_rotation % 4 == 1)
        return v3{-v.y, v.x, v.z};
    if (cardinal_rotation % 4 == 2)
        return v3{-v.x, -v.y, v.z};
    if (cardinal_rotation % 4 == 3)
        return v3{v.y, -v.x, v.z};
    return v;
}

template <> bool is_nan(v2 value) {
    return std::isnan(value.x) || std::isnan(value.y);
}
template <> bool is_nan(v3 value) {
    return std::isnan(value.x) || std::isnan(value.y) || std::isnan(value.z);
}

} // namespace math