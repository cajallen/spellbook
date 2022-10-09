#pragma once

#include "math.hpp"

#include "geometry.hpp"

namespace spellbook {

template <typename T> struct quat_ {
    union {
        struct {
            T x, y, z, w;
        };
        v3_<T> xyz;
        T      data[4];
    };

    JSON_IMPL(quat_<T>, x, y, z, w)

    constexpr quat_() : x(0), y(0), z(0), w(1) {}
    template <typename R> constexpr quat_(quat_<R> q) : x(T(q.x)), y(T(q.y)), z(T(q.z)), w(T(q.w)) {}
    constexpr explicit quat_(v3_<T> axis, T ang)
        : x(axis.x * math::sin(ang / 2)), y(axis.y * math::sin(ang / 2)), z(axis.z * math::sin(ang / 2)), w(math::cos(ang / 2)) {}
    constexpr explicit quat_(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    constexpr explicit quat_(v3_<T> vector) {
        f32 angle = math::length(vector);
        if (angle < 0.000001) {
            x = 0;
            y = 0;
            z = 0;
            w = 1;
        } else {
            auto q = quat(vector * (math::sin(angle / 2) / angle), math::cos(angle / 2));
            x      = q.x;
            y      = q.y;
            z      = q.z;
            w      = q.w;
        }
    }

    T& operator[](s32 i) {
        return data[i];
    }
    const T& operator[](s32 i) const {
        return data[i];
    }

    quat_ operator+() const {
        return quat_(+x, +y, +z, +w);
    }
    quat_ operator-() const {
        return quat_(-x, -y, -z, -w);
    }

    quat_ operator+(quat_ q) const {
        return quat_(x + q.x, y + q.y, z + q.z, w + q.w);
    }
    quat_ operator-(quat_ q) const {
        return quat_(x - q.x, y - q.y, z - q.z, w - q.w);
    }
    quat_ operator*(quat_ q) const {
        return quat_(x * q.x, y * q.y, z * q.z, w * q.w);
    }
    quat_ operator/(quat_ q) const {
        return quat_(x / q.x, y / q.y, z / q.z, w / q.w);
    }

    quat_ operator+(T s) const {
        return quat_(x + s, y + s, z + s, w + s);
    }
    quat_ operator-(T s) const {
        return quat_(x - s, y - s, z - s, w - s);
    }
    quat_ operator*(T s) const {
        return quat_(x * s, y * s, z * s, w * s);
    }
    quat_ operator/(T s) const {
        return quat_(x / s, y / s, z / s, w / s);
    }

    quat_& operator+=(quat_ q) {
        x += q.x;
        y += q.y;
        z += q.z;
        w += q.w;
        return *this;
    }
    quat_& operator-=(quat_ q) {
        x -= q.x;
        y -= q.y;
        z -= q.z;
        w -= q.w;
        return *this;
    }
    quat_& operator*=(quat_ q) {
        x *= q.x;
        y *= q.y;
        z *= q.z;
        w *= q.w;
        return *this;
    }
    quat_& operator/=(quat_ q) {
        x /= q.x;
        y /= q.y;
        z /= q.z;
        w /= q.w;
        return *this;
    }

    quat_& operator+=(T s) {
        x += s;
        y += s;
        z += s;
        w += s;
        return *this;
    }
    quat_& operator-=(T s) {
        x -= s;
        y -= s;
        z -= s;
        w -= s;
        return *this;
    }
    quat_& operator*=(T s) {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }
    quat_& operator/=(T s) {
        x /= s;
        y /= s;
        z /= s;
        w /= s;
        return *this;
    }

    bool operator==(quat_ q) const {
        return (x == q.x) && (y == q.y) && (z == q.z) && (w == q.w);
    }
    bool operator!=(quat_ q) const {
        return (x != q.x) || (y != q.y) || (z != q.z) || (w != q.w);
    }
};

template <typename T> quat_<T> operator+(T s, quat_<T> q) {
    return quat_<T>(s + q.x, s + q.y, s + q.z, s + q.w);
}
template <typename T> quat_<T> operator-(T s, quat_<T> q) {
    return quat_<T>(s - q.x, s - q.y, s - q.z, s - q.w);
}
template <typename T> quat_<T> operator*(T s, quat_<T> q) {
    return quat_<T>(s * q.x, s * q.y, s * q.z, s * q.w);
}
template <typename T> quat_<T> operator/(T s, quat_<T> q) {
    return quat_<T>(s / q.x, s / q.y, s / q.z, s / q.w);
}

typedef quat_<f32> quat;

namespace math {

quat  euler2quat(euler e);
euler quat2euler(quat q);

quat invert(quat q);
v3   rotate(quat q, v3 v);
quat rotate(quat q, quat p);
quat rotate_inv(quat q, quat p);

}

}