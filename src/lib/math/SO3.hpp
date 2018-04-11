#pragma once
#include "Math.hpp"

namespace math {

template<typename T> /* constexpr */ Matrix3x3<T> RotationXYZ(T ax, T ay, T az)
{
	float cx = std::cos(ax);
	float sx = std::sin(ax);
	float cy = std::cos(ay);
	float sy = std::sin(ay);
	float cz = std::cos(az);
	float sz = std::sin(az);
	return Matrix3x3<T>(
		cz * cy,	cz * sy * sx - sz * cx,		cz * sy * cx + sz * sx,
		sz * cy,	sz * sy * sx + cz * cx,		sz * sy * cx - cz * sx,
		-sy,		cy * sx,					cy * cx);
}

} // namespace math