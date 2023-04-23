#include "quaternion.hpp"

namespace spellbook::math {

quat to_quat(euler e) {
    float cy = math::cos(e.yaw * 0.5f);
    float sy = math::sin(e.yaw * 0.5f);
    float cp = math::cos(e.pitch * 0.5f);
    float sp = math::sin(e.pitch * 0.5f);
    float cr = math::cos(e.roll * 0.5f);
    float sr = math::sin(e.roll * 0.5f);

	return quat(sy * cp * cr - cy * sp * sr, 
                cy * sp * cr + sy * cp * sr, 
                cy * cp * sr - sy * sp * cr,
				cy * cp * cr + sy * sp * sr
    );
}

quat to_quat(v3 v) {
    auto angle = math::length(v);
    if (angle < 0.0001f) return quat();
    return quat(v * (math::sin(angle / 2.0f) / angle), math::cos(angle / 2.0f));
}

euler to_euler(quat q) {
	f32 k = 2.0f * (q.w * q.y - q.z * q.x);
	return euler{math::atan2(2.0f * (q.w * q.x + q.y * q.z), 1.0f - 2.0f * (q.x * q.x + q.y * q.y)),
				 math::abs(k) >= 1.0f ? math::copy_sign(math::PI, k) : math::asin(k),
				 math::atan2(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.y * q.y + q.z * q.z))};
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

f32 dot(quat a, quat b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

quat slerp(quat x, quat y, float t) {
    quat z = y;

    float cosTheta = math::dot(x, y);

    // If cosTheta < 0, the interpolation will take the long way around the sphere.
    // To fix this, one quat must be negated.
    if (cosTheta < 0.0f) {
        z = -y;
        cosTheta = -cosTheta;
    }

    // Perform a linear interpolation when cosTheta is close to 1 to avoid side effect of sin(angle) becoming a zero denominator
    if(cosTheta > 1.0f - std::numeric_limits<float>::epsilon())
    {
        return quat(
            mix(x.x, z.x, t),
            mix(x.y, z.y, t),
            mix(x.z, z.z, t),
            mix(x.w, z.w, t));
    }

    // Essential Mathematics, page 467
    float angle = acos(cosTheta);
    return (sin((1.0f - t) * angle) * x + sin(t * angle) * z) / sin(angle);
}

f32 length(quat v) {
    return sqrt(math::dot(v, v));
}

quat normalize(quat q) {
    return q / math::length(q);
}

quat quat_between(v3 from, v3 to) {
    quat q;
    v3 a = math::cross(from, to);
    q.xyz = a;
    q.w = math::sqrt(math::pow(math::length(from), 2.0f) * math::pow(math::length(to), 2.0f) + math::dot(from, to));
    return math::normalize(q);
}


}
