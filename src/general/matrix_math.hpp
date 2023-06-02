#pragma once

#include "matrix.hpp"
#include "quaternion.hpp"

namespace spellbook::math {

m44 perspective(f32 vhr_fov, f32 aspect_xy, f32 near);
m44 orthographic(v3 extents);
m44 look(v3 eye, v3 vec, v3 up);
m44 look_ik(v3 eye, v3 vec, v3 up);

m44 translate(v3 position);
m44 scale(v3 scale);
m44 rotation(euler e);

m44   rotation(quat q);
euler to_euler(m44 rot);
quat  to_quat(m33 rot);
quat  to_quat(m44 rot);

m33 inverse(const m33& A);
m44 inverse(const m44& A);
m44 transpose(const m44& A);
ray3 transformed_ray(const m44& transform, v2 viewport_UV);

void extract_tsr(m44 m, v3* translation, v3* scale = nullptr, quat* rotation = nullptr);

v3 apply_transform(const m44& m, const v3& v);

}

namespace spellbook {

v4   operator*(const m44& lhs, const v4& rhs);
m44  operator*(const m44& lhs, const m44& rhs);
v3   operator*(const m33& lhs, const v3& rhs);
m33  operator*(const m33& lhs, const m33& rhs);
bool operator==(const m44& lhs, const m44& rhs);
bool operator!=(const m44& lhs, const m44& rhs);

}

