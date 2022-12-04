#include "matrix_math.hpp"

#include "lib/math.hpp"

namespace spellbook::math {

m44 infinite_perspective(f32 v_fov_r, f32 aspect_xy, f32 near) {
    auto focal_length = 1.0f / math::tan(v_fov_r);

    m44 result = {focal_length / aspect_xy, 0, 0, 0, 0, -focal_length, 0, 0, 0, 0, 0, near, 0, 0, -1, 0};
    return result;
}


m44 look(v3 eye, v3 vec, v3 up) {
    // NOTE: z-axis points in the opposite direction to how depth maps
    v3 z_axis = -math::normalize(vec);
    v3 x_axis = math::normalize(math::cross(up, z_axis));
    v3 y_axis = math::cross(z_axis, x_axis);

    m44 result = m44({
        x_axis.x,
        x_axis.y,
        x_axis.z,
        -math::dot(eye, x_axis),
        y_axis.x,
        y_axis.y,
        y_axis.z,
        -math::dot(eye, y_axis),
        z_axis.x,
        z_axis.y,
        z_axis.z,
        -math::dot(eye, z_axis),
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    });

    return result;
}

m44 translate(v3 position) {
    return m44 {1, 0, 0, position.x, 0, 1, 0, position.y, 0, 0, 1, position.z, 0, 0, 0, 1};
}

m44 scale(v3 scale) {
    return m44 {scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, 0, 0, 0, 1};
}

m44 rotation(euler e) {
    auto croll  = math::cos(e.roll);
    auto cpitch = math::cos(e.pitch);
    auto cyaw   = math::cos(e.yaw);
    auto sroll  = math::sin(e.roll);
    auto spitch = math::sin(e.pitch);
    auto syaw   = math::sin(e.yaw);

    return m44 {croll * cyaw + sroll * spitch * syaw,
        -croll * syaw + sroll * spitch * cyaw,
        sroll * cpitch,
        0,
        cpitch * syaw,
        cpitch * cyaw,
        -spitch,
        0,
        -sroll * cyaw + croll * spitch * syaw,
        sroll * syaw + croll * spitch * cyaw,
        croll * cpitch,
        0,
        0,
        0,
        0,
        1};
}

euler rotation2euler(const m33& rot) {
    euler euler;
    euler.pitch = math::asin(rot.cr(2, 0));
    f32 C       = math::cos(euler.pitch);

    // Gimball lock?
    if (math::abs(C) > 0.005f) {
        // No
        f32 tr_x  = rot.cr(2, 2) / C;
        f32 tr_y  = -rot.cr(2, 1) / C;
        euler.yaw = math::atan2(tr_y, tr_x);

        tr_x       = rot.cr(0, 0) / C;
        tr_y       = -rot.cr(1, 0) / C;
        euler.roll = math::atan2(tr_y, tr_x);
    } else {
        // Yes
        euler.yaw  = 0;
        f32 tr_x   = rot.cr(1, 1);
        f32 tr_y   = rot.cr(0, 1);
        euler.roll = math::atan2(tr_y, tr_x);
    }
    return euler;
}

m44 rotation(quat q) {
    auto xx = q.x * q.x;
    auto cr = q.x * q.y;
    auto xz = q.x * q.z;
    auto xw = q.x * q.w;
    auto yy = q.y * q.y;
    auto yz = q.y * q.z;
    auto yw = q.y * q.w;
    auto zz = q.z * q.z;
    auto zw = q.z * q.w;

    return m44(1.0f - 2.0f * (yy + zz),
        2.0f * (cr - zw),
        2.0f * (xz + yw),
        0.0f,
        2.0f * (cr + zw),
        1.0f - 2.0f * (xx + zz),
        2.0f * (yz - xw),
        0.0f,
        2.0f * (xz - yw),
        2.0f * (yz + xw),
        1.0f - 2.0f * (xx + yy),
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f);
}

euler to_euler(m44 rot) {
    return to_euler(to_quat(rot));
}

quat to_quat(m33 mat) {
    // Does this work?
    quat q;
    q.w = sqrt(1 + mat.rc(1,1) + mat.rc(2,2) + mat.rc(3,3)) / 2;
    q.x = (mat.rc(3,2) - mat.rc(2,3)) / (4 * q.w);
    q.y = (mat.rc(1,3) - mat.rc(3,1)) / (4 * q.w);
    q.z = (mat.rc(2,1) - mat.rc(1,2)) / (4 * q.w);
    return q;
}

quat  to_quat(m44 rot) {
    return to_quat(m33(rot));
}



m44 inverse(const m44& A) {
    auto d0 = -A.cr(3, 2) * A.cr(2, 3) * A.cr(1, 1) + A.cr(2, 2) * A.cr(3, 3) * A.cr(1, 1) + A.cr(3, 2) * A.cr(1, 3) * A.cr(2, 1) -
              A.cr(2, 2) * A.cr(1, 3) * A.cr(3, 1) - A.cr(3, 3) * A.cr(2, 1) * A.cr(1, 2) + A.cr(2, 3) * A.cr(3, 1) * A.cr(1, 2);
    auto d1 = A.cr(3, 2) * A.cr(2, 3) * A.cr(0, 1) - A.cr(2, 2) * A.cr(3, 3) * A.cr(0, 1) - A.cr(3, 2) * A.cr(0, 3) * A.cr(2, 1) +
              A.cr(2, 2) * A.cr(0, 3) * A.cr(3, 1) + A.cr(3, 3) * A.cr(2, 1) * A.cr(0, 2) - A.cr(2, 3) * A.cr(3, 1) * A.cr(0, 2);
    auto d2 = -A.cr(3, 2) * A.cr(1, 3) * A.cr(0, 1) + A.cr(3, 2) * A.cr(0, 3) * A.cr(1, 1) - A.cr(3, 3) * A.cr(1, 1) * A.cr(0, 2) +
              A.cr(1, 3) * A.cr(3, 1) * A.cr(0, 2) + A.cr(3, 3) * A.cr(0, 1) * A.cr(1, 2) - A.cr(0, 3) * A.cr(3, 1) * A.cr(1, 2);
    auto d3 = A.cr(2, 2) * A.cr(1, 3) * A.cr(0, 1) - A.cr(2, 2) * A.cr(0, 3) * A.cr(1, 1) + A.cr(2, 3) * A.cr(1, 1) * A.cr(0, 2) -
              A.cr(1, 3) * A.cr(2, 1) * A.cr(0, 2) - A.cr(2, 3) * A.cr(0, 1) * A.cr(1, 2) + A.cr(0, 3) * A.cr(2, 1) * A.cr(1, 2);
    auto det_inv = 1.0 / (A.cr(0, 0) * d0 + A.cr(1, 0) * d1 + A.cr(2, 0) * d2 + A.cr(3, 0) * d3);

    return m44(d0 * det_inv,
        (A.cr(1, 0) * A.cr(3, 2) * A.cr(2, 3) - A.cr(1, 0) * A.cr(2, 2) * A.cr(3, 3) - A.cr(3, 2) * A.cr(1, 3) * A.cr(2, 0) +
            A.cr(2, 2) * A.cr(1, 3) * A.cr(3, 0) + A.cr(3, 3) * A.cr(2, 0) * A.cr(1, 2) - A.cr(2, 3) * A.cr(3, 0) * A.cr(1, 2)) *
            det_inv,
        (-A.cr(3, 3) * A.cr(2, 0) * A.cr(1, 1) + A.cr(2, 3) * A.cr(3, 0) * A.cr(1, 1) + A.cr(1, 0) * A.cr(3, 3) * A.cr(2, 1) -
            A.cr(1, 3) * A.cr(3, 0) * A.cr(2, 1) - A.cr(1, 0) * A.cr(2, 3) * A.cr(3, 1) + A.cr(1, 3) * A.cr(2, 0) * A.cr(3, 1)) *
            det_inv,
        (A.cr(3, 2) * A.cr(2, 0) * A.cr(1, 1) - A.cr(2, 2) * A.cr(3, 0) * A.cr(1, 1) - A.cr(1, 0) * A.cr(3, 2) * A.cr(2, 1) +
            A.cr(1, 0) * A.cr(2, 2) * A.cr(3, 1) + A.cr(3, 0) * A.cr(2, 1) * A.cr(1, 2) - A.cr(2, 0) * A.cr(3, 1) * A.cr(1, 2)) *
            det_inv,
        d1 * det_inv,
        (-A.cr(0, 0) * A.cr(3, 2) * A.cr(2, 3) + A.cr(0, 0) * A.cr(2, 2) * A.cr(3, 3) + A.cr(3, 2) * A.cr(0, 3) * A.cr(2, 0) -
            A.cr(2, 2) * A.cr(0, 3) * A.cr(3, 0) - A.cr(3, 3) * A.cr(2, 0) * A.cr(0, 2) + A.cr(2, 3) * A.cr(3, 0) * A.cr(0, 2)) *
            det_inv,
        (A.cr(3, 3) * A.cr(2, 0) * A.cr(0, 1) - A.cr(2, 3) * A.cr(3, 0) * A.cr(0, 1) - A.cr(0, 0) * A.cr(3, 3) * A.cr(2, 1) +
            A.cr(0, 3) * A.cr(3, 0) * A.cr(2, 1) + A.cr(0, 0) * A.cr(2, 3) * A.cr(3, 1) - A.cr(0, 3) * A.cr(2, 0) * A.cr(3, 1)) *
            det_inv,
        (-A.cr(3, 2) * A.cr(2, 0) * A.cr(0, 1) + A.cr(2, 2) * A.cr(3, 0) * A.cr(0, 1) + A.cr(0, 0) * A.cr(3, 2) * A.cr(2, 1) -
            A.cr(0, 0) * A.cr(2, 2) * A.cr(3, 1) - A.cr(3, 0) * A.cr(2, 1) * A.cr(0, 2) + A.cr(2, 0) * A.cr(3, 1) * A.cr(0, 2)) *
            det_inv,
        d2 * det_inv,
        (-A.cr(1, 0) * A.cr(3, 2) * A.cr(0, 3) + A.cr(0, 0) * A.cr(3, 2) * A.cr(1, 3) + A.cr(1, 0) * A.cr(3, 3) * A.cr(0, 2) -
            A.cr(1, 3) * A.cr(3, 0) * A.cr(0, 2) - A.cr(0, 0) * A.cr(3, 3) * A.cr(1, 2) + A.cr(0, 3) * A.cr(3, 0) * A.cr(1, 2)) *
            det_inv,
        (-A.cr(1, 0) * A.cr(3, 3) * A.cr(0, 1) + A.cr(1, 3) * A.cr(3, 0) * A.cr(0, 1) + A.cr(0, 0) * A.cr(3, 3) * A.cr(1, 1) -
            A.cr(0, 3) * A.cr(3, 0) * A.cr(1, 1) + A.cr(1, 0) * A.cr(0, 3) * A.cr(3, 1) - A.cr(0, 0) * A.cr(1, 3) * A.cr(3, 1)) *
            det_inv,
        (A.cr(1, 0) * A.cr(3, 2) * A.cr(0, 1) - A.cr(0, 0) * A.cr(3, 2) * A.cr(1, 1) + A.cr(3, 0) * A.cr(1, 1) * A.cr(0, 2) -
            A.cr(1, 0) * A.cr(3, 1) * A.cr(0, 2) - A.cr(3, 0) * A.cr(0, 1) * A.cr(1, 2) + A.cr(0, 0) * A.cr(3, 1) * A.cr(1, 2)) *
            det_inv,
        d3 * det_inv,
        (A.cr(1, 0) * A.cr(2, 2) * A.cr(0, 3) - A.cr(0, 0) * A.cr(2, 2) * A.cr(1, 3) - A.cr(1, 0) * A.cr(2, 3) * A.cr(0, 2) +
            A.cr(1, 3) * A.cr(2, 0) * A.cr(0, 2) + A.cr(0, 0) * A.cr(2, 3) * A.cr(1, 2) - A.cr(0, 3) * A.cr(2, 0) * A.cr(1, 2)) *
            det_inv,
        (A.cr(1, 0) * A.cr(2, 3) * A.cr(0, 1) - A.cr(1, 3) * A.cr(2, 0) * A.cr(0, 1) - A.cr(0, 0) * A.cr(2, 3) * A.cr(1, 1) +
            A.cr(0, 3) * A.cr(2, 0) * A.cr(1, 1) - A.cr(1, 0) * A.cr(0, 3) * A.cr(2, 1) + A.cr(0, 0) * A.cr(1, 3) * A.cr(2, 1)) *
            det_inv,
        (-A.cr(1, 0) * A.cr(2, 2) * A.cr(0, 1) + A.cr(0, 0) * A.cr(2, 2) * A.cr(1, 1) - A.cr(2, 0) * A.cr(1, 1) * A.cr(0, 2) +
            A.cr(1, 0) * A.cr(2, 1) * A.cr(0, 2) + A.cr(2, 0) * A.cr(0, 1) * A.cr(1, 2) - A.cr(0, 0) * A.cr(2, 1) * A.cr(1, 2)) *
            det_inv);
}

m33 inverse(const m33& A) {
    auto d0      = A[4] * A[8] - A[5] * A[7];
    auto d1      = A[5] * A[6] - A[3] * A[8];
    auto d2      = A[3] * A[7] - A[4] * A[6];
    auto det_inv = 1.0 / (A[0] * d0 + A[1] * d1 + A[2] * d2);

    return m33(d0 * det_inv,
        (A[2] * A[7] - A[1] * A[8]) * det_inv,
        (A[1] * A[5] - A[2] * A[4]) * det_inv,
        d1 * det_inv,
        (A[0] * A[8] - A[2] * A[6]) * det_inv,
        (A[2] * A[3] - A[0] * A[5]) * det_inv,
        d2 * det_inv,
        (A[1] * A[6] - A[0] * A[7]) * det_inv,
        (A[0] * A[4] - A[1] * A[3]) * det_inv);
}

constexpr m44 transpose(const m44& A) {
    return m44(A.rc(0, 0),
        A.rc(1, 0),
        A.rc(2, 0),
        A.rc(3, 0),
        A.rc(0, 1),
        A.rc(1, 1),
        A.rc(2, 1),
        A.rc(3, 1),
        A.rc(0, 2),
        A.rc(1, 2),
        A.rc(2, 2),
        A.rc(3, 2),
        A.rc(0, 3),
        A.rc(1, 3),
        A.rc(2, 3),
        A.rc(3, 3));
}

r3 transformed_ray(const m44& transform, v2 viewport_UV) {
    m44 transform_inv = math::inverse(transform);

    float mox = math::to_signed_range(viewport_UV.x, false);
    float moy = math::to_signed_range(viewport_UV.y, false);

    v4 origin_hpos = transform_inv * v4(mox, moy, 0.01f, 1.0f);
    v4 end_hpos    = transform_inv * v4(mox, moy, 1.0f, 1.0f);
    v3 origin      = origin_hpos.xyz / origin_hpos.w;
    v3 end         = end_hpos.xyz / end_hpos.w;
    return r3 {origin, math::normalize(end - origin)};
}

void extract_tsr(m44 m, v3* translation, v3* scale, quat* rotation) {
    if (translation != nullptr) {
        *translation = v3(m.cr(3,0), m.cr(3,1), m.cr(3,2));
    }
    v3 used_scale = v3(
        math::length(v3(m.cr(0,0), m.cr(0,1), m.cr(0,2))),
        math::length(v3(m.cr(1,0), m.cr(1,1), m.cr(1,2))),
        math::length(v3(m.cr(2,0), m.cr(2,1), m.cr(2,2))));
    if (scale != nullptr) {
        *scale = used_scale;
    }
    if (rotation != nullptr) {
        m.cr(0,0) /= used_scale.x;
        m.cr(0,1) /= used_scale.x;
        m.cr(0,2) /= used_scale.x;
    
        m.cr(1,0) /= used_scale.y;
        m.cr(1,1) /= used_scale.y;
        m.cr(1,2) /= used_scale.y;
    
        m.cr(2,0) /= used_scale.z;
        m.cr(2,1) /= used_scale.z;
        m.cr(2,2) /= used_scale.z;
        *rotation = -to_quat(m);
    }
}

}

namespace spellbook {

v4 operator*(const m44& lhs, const v4& rhs) {
    return v4(lhs.cr(0, 0) * rhs.x + lhs.cr(1, 0) * rhs.y + lhs.cr(2, 0) * rhs.z + lhs.cr(3, 0) * rhs.w,
        lhs.cr(0, 1) * rhs.x + lhs.cr(1, 1) * rhs.y + lhs.cr(2, 1) * rhs.z + lhs.cr(3, 1) * rhs.w,
        lhs.cr(0, 2) * rhs.x + lhs.cr(1, 2) * rhs.y + lhs.cr(2, 2) * rhs.z + lhs.cr(3, 2) * rhs.w,
        lhs.cr(0, 3) * rhs.x + lhs.cr(1, 3) * rhs.y + lhs.cr(2, 3) * rhs.z + lhs.cr(3, 3) * rhs.w);
}

m44 operator*(const m44& lhs, const m44& rhs) {
    m44 out = m44();
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            for (int u = 0; u < 4; u++)
                out.cr(j, i) += lhs.cr(u, i) * rhs.cr(j, u);
        }
    return out;
}

v3 operator*(const m33& lhs, const v3& rhs) {
    return v3(lhs.cr(0, 0) * rhs.x + lhs.cr(1, 0) * rhs.y + lhs.cr(2, 0) * rhs.z,
        lhs.cr(0, 1) * rhs.x + lhs.cr(1, 1) * rhs.y + lhs.cr(2, 1) * rhs.z,
        lhs.cr(0, 2) * rhs.x + lhs.cr(1, 2) * rhs.y + lhs.cr(2, 2) * rhs.z);
}

m33 operator*(const m33& lhs, const m33& rhs) {
    m33 out = m33();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            for (int u = 0; u < 3; u++)
                out.cr(j, i) += lhs.cr(u, i) * rhs.cr(j, u);
        }
    return out;
}

bool operator==(const m44& lhs, const m44& rhs) {
    for (int i = 0; i < 16; i++)
        if (lhs[i] != rhs[i])
            return false;
    return true;
}

bool operator!=(const m44& lhs, const m44& rhs) {
    return !(lhs == rhs);
}

}
