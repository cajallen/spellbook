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

    constexpr quat_() : x(0), y(0), z(0), w(1) {}
    template <typename R> constexpr quat_(quat_<R> q) : x(T(q.x)), y(T(q.y)), z(T(q.z)), w(T(q.w)) {}
    constexpr explicit quat_(v3_<T> axis, T ang) : x(axis.x * math::sin(ang / 2)), y(axis.y * math::sin(ang / 2)), z(axis.z * math::sin(ang / 2)), w(math::cos(ang / 2)) {
        float len = sqrt(x * x + y * y + z * z + w * w);
        x /= len;
        y /= len;
        z /= len;
        w /= len;
    }
    constexpr explicit quat_(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    constexpr explicit quat_(v3_<T> vector) {
        f32 angle = math::length(vector);
        if (angle < 0.00001f) {
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
        float x2 = w * q.x + x * q.w + y * q.z - z * q.y;
        float y2 = w * q.y - x * q.z + y * q.w + z * q.x;
        float z2 = w * q.z + x * q.y - y * q.x + z * q.w;
        float w2 = w * q.w - x * q.x - y * q.y - z * q.z;

        return quat_(x2,y2,z2,w2);
        
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

JSON_IMPL(quat, x, y, z, w);

namespace math {

quat to_quat(const v3& v);
quat  to_quat(const euler& e);
euler to_euler(const quat& q);

quat invert(const quat& q);
v3   rotate(const quat& q, const v3& v);
quat rotate(const quat& q, const quat& p);
quat rotate_inv(const quat& q, const quat& p);
f32 dot(const quat& q1, const quat& q2);
f32 length(const quat& q);
quat normalize(const quat& q);
quat slerp(const quat& q1, const quat& q2, float t);

quat quat_between(const v3& from, const v3& to);

}

euler string2quat(const string& word);

}