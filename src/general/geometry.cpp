#include "geometry.hpp"

#include <vuk/Types.hpp>

#include "math.hpp"
#include "logger.hpp"

namespace spellbook {

euler euler::r2d() {
    return {math::r2d(yaw), math::r2d(pitch), math::r2d(roll)};
}
euler euler::d2r() {
    return {math::d2r(yaw), math::d2r(pitch), math::d2r(roll)};
}

line_<f32>::operator v2() const {
    return v2{start, end};
}


constexpr v2_<s32>::v2_(vuk::Extent2D vk_extent) {
    x = vk_extent.width; y = vk_extent.height;
}
constexpr v2_<s32>::v2_(vuk::Extent3D vk_extent) {
    x = vk_extent.width; y = vk_extent.height;
}
constexpr v3_<s32>::v3_(vuk::Extent2D vk_extent) {
    x = vk_extent.width; y = vk_extent.height; z = 1;
}
constexpr v3_<s32>::v3_(vuk::Extent3D vk_extent) {
    x = vk_extent.width; y = vk_extent.height; z = vk_extent.depth;
}

constexpr v2_<f32>::v2_(vuk::Extent2D vk_extent) {
    x = vk_extent.width; y = vk_extent.height;
}
constexpr v2_<f32>::v2_(vuk::Extent3D vk_extent) {
    x = vk_extent.width; y = vk_extent.height;
}
constexpr v3_<f32>::v3_(vuk::Extent2D vk_extent) {
    x = vk_extent.width; y = vk_extent.height; z = 1;
}
constexpr v3_<f32>::v3_(vuk::Extent3D vk_extent) {
    x = vk_extent.width; y = vk_extent.height; z = vk_extent.depth;
}

constexpr v2_<f32>::operator vuk::Extent2D() const {
    return vuk::Extent2D{(u32) math::round(x), (u32) math::round(y)};
}

constexpr v2_<f32>::operator vuk::Extent3D() const {
    return vuk::Extent3D{(u32) math::round(x), (u32) math::round(y), 1};
}

constexpr v2_<s32>::operator vuk::Extent2D() const {
    return vuk::Extent2D{(u32) x, (u32) y};
}

constexpr v2_<s32>::operator vuk::Extent3D() const {
    return vuk::Extent3D{(u32) x, (u32) y, 1};
}

constexpr v3_<f32>::operator vuk::Extent2D() const {
    return vuk::Extent2D{(u32) math::round(x), (u32) math::round(y)};
}

constexpr v3_<f32>::operator vuk::Extent3D() const {
    return vuk::Extent3D{(u32) math::round(x), (u32) math::round(y), (u32) math::round(z)};
}

constexpr v3_<s32>::operator vuk::Extent2D() const {
    return vuk::Extent2D{(u32) x, (u32) y};
}

constexpr v3_<s32>::operator vuk::Extent3D() const {
    return vuk::Extent3D{(u32) x, (u32) y, (u32) z};
}

v2 string2v2(string word) {
    int first = word.find_first_of(',');
    int last = word.find_last_of(',');
    if (first != last)
        return v2(0);
    return v2(
        std::stof(word.substr(0, first)),
        std::stof(word.substr(first + 1))
    );
}

v2i string2v2i(string word) {
    int first = word.find_first_of(',');
    int last = word.find_last_of(',');
    if (first != last)
        return v2i(0);
    return v2i(
        std::stoi(word.substr(0, first)),
        std::stoi(word.substr(first + 1))
    );
}

v3 string2v3(string word) {
    int first = word.find_first_of(',');
    int second = word.find_first_of(',', first + 1);
    int last = word.find_last_of(',');
    if (second != last)
        return v3(0);
    return v3(
        std::stof(word.substr(0, first)),
        std::stof(word.substr(first + 1, second - (first + 1))),
        std::stof(word.substr(second))
    );
}

v4 string2v4(string word) {
    int first = word.find_first_of(',');
    int second = word.find_first_of(',', first + 1);
    int third = word.find_first_of(',', second + 1);
    int last = word.find_last_of(',');
    if (third != last)
        return v4(0);
    return v4(
        std::stof(word.substr(0, first)),
        std::stof(word.substr(first + 1, second - (first + 1))),
        std::stof(word.substr(second + 1, third - (second + 1))),
        std::stof(word.substr(second))
    );
}

euler string2euler(string word) {
    // default is DEG currently
    enum { DEFAULT, DEG, RAD };
    int units = word[0] == 'r' ? RAD : 
                word[0] == 'd' ? DEG : 
                DEFAULT;

    int offset = units == DEFAULT ? 0 : 1;

    int first = word.find_first_of(',');
    int last = word.find_last_of(',');
    if (first != last)
        return euler{};
    return euler{
        std::stof(word.substr(offset, first - offset)) * (units == RAD ? 1.0f : math::D2R),
        std::stof(word.substr(first + 1))              * (units == RAD ? 1.0f : math::D2R)
    };
}


v3i direction_to_vec(Direction direction) {
    switch (direction) {
        case (Direction_PosX): return v3i::X;
        case (Direction_NegX): return -v3i::X;
        case (Direction_PosY): return v3i::Y;
        case (Direction_NegY): return -v3i::Y;
        case (Direction_PosZ): return v3i::Z;
        case (Direction_NegZ): return -v3i::Z;
        default: {
            assert_else(false)
                return v3i::X;
        }
    }
}

Direction flip_direction(Direction direction) {
    switch (direction) {
        case (Direction_PosX): return Direction_NegX;
        case (Direction_NegX): return Direction_PosX;
        case (Direction_PosY): return Direction_NegY;
        case (Direction_NegY): return Direction_PosY;
        case (Direction_PosZ): return Direction_NegZ;
        case (Direction_NegZ): return Direction_PosZ;
        default: {
            assert_else(false)
                return Direction_NegX;
        }
    }
}


Direction vec_to_direction(v3i vec) {
    if (vec == v3i::X)
        return Direction_PosX;
    if (vec == -v3i::X)
        return Direction_NegX;
    if (vec == v3i::Y)
        return Direction_PosY;
    if (vec == -v3i::Y)
        return Direction_NegY;
    if (vec == v3i::Z)
        return Direction_PosZ;
    if (vec == -v3i::Z)
        return Direction_NegZ;

    assert_else(false)
        return Direction_PosX;
}

}
