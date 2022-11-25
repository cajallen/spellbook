#pragma once

#include "lib/string.hpp"
#include "lib/json.hpp"
#include "lib/hash.hpp"

namespace vuk {
struct Extent2D;
struct Extent3D;
}

namespace spellbook {

enum Direction {
    Direction_Up       = 1 << 0,
    Direction_Down     = 1 << 1,
    Direction_Left     = 1 << 2,
    Direction_Right    = 1 << 3,
    Direction_Forward  = 1 << 4,
    Direction_Backward = 1 << 5,
};

template <typename T> struct v2_ {
    union {
        struct {
            T x, y;
        };
        struct {
            T r, g;
        };
        T data[2];
    };

    v2_() : x(), y() {}
    constexpr explicit v2_(T s) {
        x = s;
        y = s;
    }
    template <typename S> explicit v2_(v2_<S> s) {
        x = (T) s.x;
        y = (T) s.y;
    }
    constexpr v2_(T s, T t) {
        x = s;
        y = t;
    }
    constexpr explicit v2_(vuk::Extent2D vk_extent);
    constexpr explicit v2_(vuk::Extent3D vk_extent);

    constexpr T& operator[](s32 i) {
        return data[i];
    }
    const constexpr T& operator[](s32 i) const {
        return data[i];
    }

    constexpr v2_ operator+() const {
        return v2_(+x, +y);
    }
    constexpr v2_ operator-() const {
        return v2_(-x, -y);
    }

    constexpr v2_ operator+(v2_ v) const {
        return v2_(x + v.x, y + v.y);
    }
    constexpr v2_ operator-(v2_ v) const {
        return v2_(x - v.x, y - v.y);
    }
    constexpr v2_ operator*(v2_ v) const {
        return v2_(x * v.x, y * v.y);
    }
    constexpr v2_ operator/(v2_ v) const {
        return v2_(x / v.x, y / v.y);
    }

    template <typename S> constexpr v2_ operator*(v2_<S> s_v) const {
        auto v = v2_<T>(s_v);
        return v2_(x * v.x, y * v.y);
    }

    constexpr v2_ operator+(T s) const {
        return v2_(x + s, y + s);
    }
    constexpr v2_ operator-(T s) const {
        return v2_(x - s, y - s);
    }
    constexpr v2_ operator*(T s) const {
        return v2_(x * s, y * s);
    }
    constexpr v2_ operator/(T s) const {
        return v2_(x / s, y / s);
    }

    constexpr v2_& operator+=(v2_ v) {
        x += v.x;
        y += v.y;
        return *this;
    }
    constexpr v2_& operator-=(v2_ v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }
    constexpr v2_& operator*=(v2_ v) {
        x *= v.x;
        y *= v.y;
        return *this;
    }
    constexpr v2_& operator/=(v2_ v) {
        x /= v.x;
        y /= v.y;
        return *this;
    }

    constexpr v2_& operator+=(T s) {
        x += s;
        y += s;
        return *this;
    }
    constexpr v2_& operator-=(T s) {
        x -= s;
        y -= s;
        return *this;
    }
    constexpr v2_& operator*=(T s) {
        x *= s;
        y *= s;
        return *this;
    }
    constexpr v2_& operator/=(T s) {
        x /= s;
        y /= s;
        return *this;
    }

    constexpr bool operator==(const v2_& v) const {
        return (x == v.x) && (y == v.y);
    }
    constexpr bool operator!=(const v2_& v) const {
        return (x != v.x) || (y != v.y);
    }

    constexpr operator vuk::Extent2D() const;
    constexpr operator vuk::Extent3D() const;

    static constexpr s32 DIM = 2;
    static const v2_     X;
    static const v2_     Y;
};
template <typename T> const v2_<T> v2_<T>::X(1, 0);
template <typename T> const v2_<T> v2_<T>::Y(0, 1);

JSON_IMPL_TEMPLATE(template<typename T>, v2_<T>, x, y);

template <typename T> constexpr v2_<T> operator+(T s, v2_<T> v) {
    return v2_<T>(s + v.x, s + v.y);
}
template <typename T> constexpr v2_<T> operator-(T s, v2_<T> v) {
    return v2_<T>(s - v.x, s - v.y);
}
template <typename T> constexpr v2_<T> operator*(T s, v2_<T> v) {
    return v2_<T>(s * v.x, s * v.y);
}
template <typename T> constexpr v2_<T> operator/(T s, v2_<T> v) {
    return v2_<T>(s / v.x, s / v.y);
}

template <typename T> struct v3_ {
    union {
        T data[3];
        struct {
            T x, y, z;
        };
        struct {
            T r, g, b;
        };

        v2_<T> xy;
        struct {
            T      ignore;
            v2_<T> yz;
        };
    };

    v3_() : x(), y(), z() {}
    constexpr v3_(T s) {
        x = s;
        y = s;
        z = s;
    }
    template <typename S> constexpr explicit v3_(v3_<S> s) {
        x = (T) s.x;
        y = (T) s.y;
        z = (T) s.z;
    }
    constexpr v3_(T s, T t, T v) {
        x = s;
        y = t;
        z = v;
    }
    constexpr v3_(v2_<T> v, T s) {
        x = v.x;
        y = v.y;
        z = s;
    }
    constexpr explicit v3_(vuk::Extent2D vk_extent);
    constexpr explicit v3_(vuk::Extent3D vk_extent);

    constexpr T& operator[](s32 i) {
        return data[i];
    }
    const constexpr T& operator[](s32 i) const {
        return data[i];
    }

    constexpr v3_ operator+() const {
        return v3_(+x, +y, +z);
    }
    constexpr v3_ operator-() const {
        return v3_(-x, -y, -z);
    }

    constexpr v3_ operator+(v3_ v) const {
        return v3_(x + v.x, y + v.y, z + v.z);
    }
    constexpr v3_ operator-(v3_ v) const {
        return v3_(x - v.x, y - v.y, z - v.z);
    }
    constexpr v3_ operator*(v3_ v) const {
        return v3_(x * v.x, y * v.y, z * v.z);
    }
    constexpr v3_ operator/(v3_ v) const {
        return v3_(x / v.x, y / v.y, z / v.z);
    }

    constexpr v3_ operator+(T s) const {
        return v3_(x + s, y + s, z + s);
    }
    constexpr v3_ operator-(T s) const {
        return v3_(x - s, y - s, z - s);
    }
    constexpr v3_ operator*(T s) const {
        return v3_(x * s, y * s, z * s);
    }
    constexpr v3_ operator/(T s) const {
        return v3_(x / s, y / s, z / s);
    }

    constexpr v3_& operator+=(v3_ v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    constexpr v3_& operator-=(v3_ v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    constexpr v3_& operator*=(v3_ v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }
    constexpr v3_& operator/=(v3_ v) {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }

    constexpr v3_& operator+=(T s) {
        x += s;
        y += s;
        z += s;
        return *this;
    }
    constexpr v3_& operator-=(T s) {
        x -= s;
        y -= s;
        z -= s;
        return *this;
    }
    constexpr v3_& operator*=(T s) {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    constexpr v3_& operator/=(T s) {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    constexpr bool operator==(const v3_& v) const {
        return (x == v.x) && (y == v.y) && (z == v.z);
    }
    constexpr bool operator!=(const v3_& v) const {
        return (x != v.x) || (y != v.y) || (z != v.z);
    }

    constexpr operator vuk::Extent2D() const;
    constexpr operator vuk::Extent3D() const;

    static constexpr s32 DIM = 3;
    static const v3_     X;
    static const v3_     Y;
    static const v3_     Z;
};
template <typename T> const v3_<T> v3_<T>::X(1, 0, 0);
template <typename T> const v3_<T> v3_<T>::Y(0, 1, 0);
template <typename T> const v3_<T> v3_<T>::Z(0, 0, 1);

JSON_IMPL_TEMPLATE(template<typename T>, v3_<T>, x, y, z)

template <typename T> constexpr v3_<T> operator+(T s, v3_<T> v) {
    return v3_<T>(s + v.x, s + v.y, s + v.z);
}
template <typename T> constexpr v3_<T> operator-(T s, v3_<T> v) {
    return v3_<T>(s - v.x, s - v.y, s - v.z);
}
template <typename T> constexpr v3_<T> operator*(T s, v3_<T> v) {
    return v3_<T>(s * v.x, s * v.y, s * v.z);
}
template <typename T> constexpr v3_<T> operator/(T s, v3_<T> v) {
    return v3_<T>(s / v.x, s / v.y, s / v.z);
}

template <typename T> struct v4_ {
    union {
        T data[4];
        struct {
            T x, y, z, w;
        };
        struct {
            T r, g, b, a;
        };

        v2_<T> xy;
        v3_<T> xyz;
        struct {
            T      ignore;
            v3_<T> yzw;
        };
        struct {
            T      ignore1;
            T      ignore2;
            v2_<T> zw;
        };
    };

    v4_() : x(), y(), z(), w() {}
    constexpr v4_(T s) {
        x = s;
        y = s;
        z = s;
        w = s;
    }
    constexpr v4_(T s, T t, T v, T u) {
        x = s;
        y = t;
        z = v;
        w = u;
    }
    constexpr v4_(v2_<T> v, T s, T t) {
        x = v.x;
        y = v.y;
        z = s;
        w = t;
    }
    constexpr v4_(v2_<T> v, v2_<T> u) {
        x = v.x;
        y = v.y;
        z = u.x;
        w = u.y;
    }
    constexpr v4_(v3_<T> v, T u) {
        x = v.x;
        y = v.y;
        z = v.z;
        w = u;
    }

    constexpr T& operator[](s32 i) {
        return data[i];
    }
    const constexpr T& operator[](s32 i) const {
        return data[i];
    }

    constexpr v4_ operator+() const {
        return v4_(+x, +y, +z, +w);
    }
    constexpr v4_ operator-() const {
        return v4_(-x, -y, -z, -w);
    }

    constexpr v4_ operator+(v4_ v) const {
        return v4_(x + v.x, y + v.y, z + v.z, w + v.w);
    }
    constexpr v4_ operator-(v4_ v) const {
        return v4_(x - v.x, y - v.y, z - v.z, w - v.w);
    }
    constexpr v4_ operator*(v4_ v) const {
        return v4_(x * v.x, y * v.y, z * v.z, w * v.w);
    }
    constexpr v4_ operator/(v4_ v) const {
        return v4_(x / v.x, y / v.y, z / v.z, w / v.w);
    }

    constexpr v4_ operator+(T s) const {
        return v4_(x + s, y + s, z + s, w + s);
    }
    constexpr v4_ operator-(T s) const {
        return v4_(x - s, y - s, z - s, w - s);
    }
    constexpr v4_ operator*(T s) const {
        return v4_(x * s, y * s, z * s, w * s);
    }
    constexpr v4_ operator/(T s) const {
        return v4_(x / s, y / s, z / s, w / s);
    }

    constexpr v4_& operator+=(v4_ v) {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }
    constexpr v4_& operator-=(v4_ v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }
    constexpr v4_& operator*=(v4_ v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        w *= v.w;
        return *this;
    }
    constexpr v4_& operator/=(v4_ v) {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        w /= v.w;
        return *this;
    }

    constexpr v4_& operator+=(T s) {
        x += s;
        y += s;
        z += s;
        w += s;
        return *this;
    }
    constexpr v4_& operator-=(T s) {
        x -= s;
        y -= s;
        z -= s;
        w -= s;
        return *this;
    }
    constexpr v4_& operator*=(T s) {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }
    constexpr v4_& operator/=(T s) {
        x /= s;
        y /= s;
        z /= s;
        w /= s;
        return *this;
    }

    constexpr bool operator==(const v4_& v) const {
        return (x == v.x) && (y == v.y) && (z == v.z) && (w == v.w);
    }
    constexpr bool operator!=(const v4_& v) const {
        return (x != v.x) || (y != v.y) || (z != v.z) || (w != v.w);
    }

    static constexpr s32 DIM = 4;
    static const v4_     X;
    static const v4_     Y;
    static const v4_     Z;
};
template <typename T> const v4_<T> v4_<T>::X(1, 0, 0, 0);
template <typename T> const v4_<T> v4_<T>::Y(0, 1, 0, 0);
template <typename T> const v4_<T> v4_<T>::Z(0, 0, 1, 0);

JSON_IMPL_TEMPLATE(template<typename T>, v4_<T>, x, y, z, w)

template <typename T> constexpr v4_<T> operator+(T s, v4_<T> p) {
    return v4_<T>(s + p.x, s + p.y, s + p.z, s + p.w);
}
template <typename T> constexpr v4_<T> operator-(T s, v4_<T> p) {
    return v4_<T>(s - p.x, s - p.y, s - p.z, s - p.w);
}
template <typename T> constexpr v4_<T> operator*(T s, v4_<T> p) {
    return v4_<T>(s * p.x, s * p.y, s * p.z, s * p.w);
}
template <typename T> constexpr v4_<T> operator/(T s, v4_<T> p) {
    return v4_<T>(s / p.x, s / p.y, s / p.z, s / p.w);
}

typedef v2_<f32> v2;
typedef v2_<s32> v2i;

typedef v3_<f32> v3;
typedef v3_<s32> v3i;

typedef v4_<f32> v4;
typedef v4_<s32> v4i;

template <typename T> struct ray_ {
    T origin;
    T dir;

    constexpr ray_() {}
    constexpr ray_(T o, T d) : origin(o), dir(d) {}
};

template <typename T> struct line_ {
    T start;
    T end;

    constexpr line_() {}
    constexpr line_(T s, T e) : start(s), end(e) {}
    constexpr line_(ray_<T>& r) {
        start = r.origin;
        end   = r.origin + r.dir;
    }
    constexpr T vector() const {
        return end - start;
    }
    constexpr bool operator==(line_<T> rhs) {
        return start == rhs.start && end == rhs.end;
    }
    constexpr bool operator!=(line_<T> rhs) {
        return start != rhs.start || end != rhs.end;
    }

    explicit operator v2() const;
};

JSON_IMPL_TEMPLATE(template <typename T>, line_<T>, start, end);

typedef ray_<v2> r2;
typedef ray_<v3> r3;

typedef line_<f32> range;
typedef line_<v2>  range2;
typedef line_<v2i> range2i;
typedef line_<v3>  range3;
typedef line_<v2>  l2;
typedef line_<v3>  l3;
typedef line_<v4>  l4;

struct euler {
    // TODO: add 0 direction static const
    union {
        struct {
            f32 yaw, pitch, roll;
        };
        f32 data[3];
    };
    
    constexpr euler operator*(f32 f) {
        return {yaw * f, pitch * f, roll * f};
    }

    constexpr euler inverted() {
        return {-yaw, -pitch, 0};
    }

    euler r2d();
    euler d2r();
};

JSON_IMPL(euler, yaw, pitch, roll);

enum AXIS { X, Y, Z };

v2    string2v2(string word);
v2i   string2v2i(string word);
v3    string2v3(string word);
v4    string2v4(string word);
euler string2euler(string word);



inline v3_<f32> operator%(v3_<f32> v1, v3_<f32> v2) {
    return v3_(std::fmod(v1.x, v2.x), std::fmod(v1.y, v2.y), std::fmod(v1.z, v2.z));
}
inline v3_<f32> operator%(v3_<f32> v, f32 s) {
    return v3_(std::fmod(v.x, s), std::fmod(v.y, s), std::fmod(v.z, s));
}


}

namespace std {

template <> struct less<spellbook::v2i> {
    bool operator()(const spellbook::v2i& lhs, const spellbook::v2i& rhs) const {
        if (lhs.x == rhs.x)
            return lhs.y < rhs.y;
        return lhs.x < rhs.x;
    }
};


template <typename T>
struct hash<spellbook::v2_<T>> {
    u64 operator()(const spellbook::v2_<T>& vec) const {
        return spellbook::hash_data(vec.data, sizeof(spellbook::v2_<T>));
    }
};

template <typename T>
struct hash<spellbook::v3_<T>> {
    u64 operator()(const spellbook::v3_<T>& vec) const {
        return spellbook::hash_data(vec.data, sizeof(spellbook::v3_<T>));
    }
};

}