#pragma once

#include "lib/matrix.hpp"
#include "lib/quaternion.hpp"

namespace spellbook::math {

m44 infinite_perspective(f32 vhr_fov, f32 aspect_xy, f32 near);
m44 look(v3 eye, v3 vec, v3 up);

m44 translate(v3 position);
m44 scale(v3 scale);
m44 rotation(euler e);

euler rotation2euler(m33 rot);
m33   quat2rotation(quat q);
m44   quat2rotation44(quat q);

m33           inverse(const m33& A);
m44           inverse(const m44& A);
constexpr m44 transpose(const m44& A);
r3            transformed_ray(const m44& transform, v2 viewport_UV);

}

namespace spellbook {

v4   operator*(const m44& lhs, const v4& rhs);
m44  operator*(const m44& lhs, const m44& rhs);
v3   operator*(const m33& lhs, const v3& rhs);
m33  operator*(const m33& lhs, const m33& rhs);
bool operator==(const m44& lhs, const m44& rhs);
bool operator!=(const m44& lhs, const m44& rhs);

}

