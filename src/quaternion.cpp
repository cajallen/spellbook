#include "quaternion.hpp"

namespace spellbook::math {

quat euler2quat(euler e) {
    double cy = cos(e.yaw * 0.5);
    double sy = sin(e.yaw * 0.5);
    double cp = cos(e.pitch * 0.5);
    double sp = sin(e.pitch * 0.5);
    double cr = cos(e.roll * 0.5);
    double sr = sin(e.roll * 0.5);

	return quat(sy * cp * cr - cy * sp * sr, 
                cy * sp * cr + sy * cp * sr, 
                cy * cp * sr - sy * sp * cr,
				cy * cp * cr + sy * sp * sr
    );
}

euler quat2euler(quat q) {
	f32 k = 2 * (q.w * q.y - q.z * q.x);
	return euler(math::atan2(2 * (q.w * q.x + q.y * q.z), 1 - 2 * (q.x * q.x + q.y * q.y)),
				 math::abs(k) >= 1 ? math::copy_sign(math::PI, k) : math::asin(k),
				 math::atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z)));
}

quat invert(quat q) {
	return quat(-q.xyz, q.w);
}

v3 rotate(quat q, v3 v) {
	v3 u = q.xyz;
	return 2.0f * u * math::dot(u, v) + v * (q.w * q.w - math::dot(u, u)) + 2.0f * q.w * math::cross(u, v);
}

quat rotate(quat q, quat p) {
	return quat(q.w * p.x + q.x * p.w + q.y * p.z - q.z * p.y, q.w * p.y - q.x * p.z + q.y * p.w + q.z * p.x,
				q.w * p.z + q.x * p.y - q.y * p.x + q.z * p.w, q.w * p.w - q.x * p.x - q.y * p.y - q.z * p.z);
}

quat rotate_inv(quat q, quat p) {
	return quat(q.w * p.x - q.x * p.w - q.y * p.z + q.z * p.y, q.w * p.y + q.x * p.z - q.y * p.w - q.z * p.x,
				q.w * p.z - q.x * p.y + q.y * p.x - q.z * p.w, q.w * p.w + q.x * p.x + q.y * p.y + q.z * p.z);
}

}
