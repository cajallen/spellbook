#pragma once

#include <utility>
#include <array>

#include "geometry.hpp"
#include "vector.hpp"
#include "json.hpp"

using std::array;
using std::exchange;
using std::move;

namespace spellbook {

struct m44 {
    array<f32, 16> data;

    m44() = default;

    m44(f32* array) {
        for (s32 i = 0; i < 16; i++)
            data[i] = array[i];
    }

    // 3x3 Constructor
    constexpr m44(f32 _00, f32 _01, f32 _02, f32 _10, f32 _11, f32 _12, f32 _20, f32 _21, f32 _22)
        : data {_00, _01, _02, 0, _10, _11, _12, 0, _20, _21, _22, 0, 0, 0, 0, 1} {}

    // 4x4 Constructor
    constexpr m44(f32 _00, f32 _01, f32 _02, f32 _03, f32 _10, f32 _11, f32 _12, f32 _13, f32 _20, f32 _21, f32 _22, f32 _23, f32 _30,
        f32 _31, f32 _32, f32 _33)
        : data {_00, _01, _02, _03, _10, _11, _12, _13, _20, _21, _22, _23, _30, _31, _32, _33} {}

    static constexpr m44 identity() {
        m44 mat = {};
        for (s32 i = 0; i < 4; i++)
            mat[i * 4 + i] = 1;
        return mat;
    }

    constexpr f32& rc(s32 r, s32 c) {
        return data[r * 4 + c];
    }
    constexpr f32 rc(s32 r, s32 c) const {
        return data[r * 4 + c];
    }

    constexpr f32& cr(s32 c, s32 r) {
        return data[r * 4 + c];
    }
    constexpr f32 cr(s32 c, s32 r) const {
        return data[r * 4 + c];
    }

    constexpr f32& operator[](s32 index) {
        return data[index];
    }

    constexpr const f32& operator[](s32 index) const {
        return data[index];
    }

    m44 operator-(m44 other) const {
        m44 out;
        for (s32 i = 0; i < 16; i++)
            out[i] = data[i] - other.data[i];
        return out;
    }

    m44 operator+(m44 other) const {
        m44 out;
        for (s32 i = 0; i < 16; i++)
            out[i] = data[i] + other.data[i];
        return out;
    }
    constexpr f32* ptr() {
        return data.data();
    }
};

inline m44 from_jv_impl(const json_value& jv, m44* _) {
    auto vec = from_jv<vector<f32>>(jv);
    m44 m;
    std::copy_n(vec.begin(), 16, m.data.begin());
    return m;
}
inline json_value to_jv(const m44& m) {
    vector<f32> vec(m.data.data(), m.data.data() + m.data.size());
    return to_jv(vec);
}

struct m33 {
    array<f32, 9> data = {};

    m33() = default;

    m33(f32* array) {
        for (s32 i = 0; i < 9; i++)
            data[i] = array[i];
    }

    // 2x2 Constructor
    constexpr m33(f32 _00, f32 _01, f32 _10, f32 _11) : data {_00, _01, _10, _11} {}

    // 3x3 Constructor
    constexpr m33(f32 _00, f32 _01, f32 _02, f32 _10, f32 _11, f32 _12, f32 _20, f32 _21, f32 _22)
        : data {_00, _01, _02, _10, _11, _12, _20, _21, _22} {}

    constexpr m33(const m44& other)
        : data {other.rc(0, 0),
              other.rc(0, 1),
              other.rc(0, 2),
              other.rc(1, 0),
              other.rc(1, 1),
              other.rc(1, 2),
              other.rc(2, 0),
              other.rc(2, 1),
              other.rc(2, 2)} {}

    constexpr static m33 identity() {
        m33 mat = {};
        for (s32 i = 0; i < 3; i++)
            mat[i * 3 + i] = 1;
        return mat;
    }

    constexpr f32& rc(s32 r, s32 c) {
        return data[r * 3 + c];
    }
    constexpr f32 rc(s32 r, s32 c) const {
        return data[r * 3 + c];
    }

    constexpr f32& cr(s32 c, s32 r) {
        return data[r * 3 + c];
    }

    constexpr f32 cr(s32 c, s32 r) const {
        return data[r * 3 + c];
    }

    constexpr f32& operator[](s32 index) {
        return data[index];
    }

    constexpr const f32& operator[](s32 index) const {
        return data[index];
    }

    constexpr m33 operator-(m33 other) const {
        m33 out;
        for (s32 i = 0; i < 9; i++)
            out[i] = data[i] - other.data[i];
        return out;
    }

    constexpr m33 operator+(m33 other) const {
        m33 out;
        for (s32 i = 0; i < 9; i++)
            out[i] = data[i] + other.data[i];
        return out;
    }

    constexpr operator m44() const {
        return m44(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
    }
};

struct m44GPU {
    array<f32, 16> data = {};
    explicit constexpr m44GPU(const m44& m) {
        data = {m.data[0],
            m.data[4],
            m.data[8],
            m.data[12],
            m.data[1],
            m.data[5],
            m.data[9],
            m.data[13],
            m.data[2],
            m.data[6],
            m.data[10],
            m.data[14],
            m.data[3],
            m.data[7],
            m.data[11],
            m.data[15]};
    } 
    constexpr m44GPU() {
        data = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    }

    explicit constexpr operator m44() const {
        return m44{data[0],
            data[4],
            data[8],
            data[12],
            data[1],
            data[5],
            data[9],
            data[13],
            data[2],
            data[6],
            data[10],
            data[14],
            data[3],
            data[7],
            data[11],
            data[15]};
    }
};

struct m34 {
    array<f32, 12> data;
    explicit constexpr m34(const m44& m) {
        data = {m.data[0],
            m.data[1],
            m.data[2],
            m.data[3],
            m.data[4],
            m.data[5],
            m.data[6],
            m.data[7],
            m.data[8],
            m.data[9],
            m.data[10],
            m.data[11]};
    }
};

struct m34GPU {
    array<f32, 12> data;
    explicit constexpr m34GPU(const m34& m) {
        data = {m.data[0],
            m.data[4],
            m.data[8],
            m.data[1],
            m.data[5],
            m.data[9],
            m.data[2],
            m.data[6],
            m.data[10],
            m.data[3],
            m.data[7],
            m.data[11]};
    }
};

}
