#pragma once
#include <cassert>

namespace math {

template<typename T> constexpr T PI = T(3.14159265358979323846L);

//=====================================================

template<typename T> struct Vector2
{
	T x, y;
	T& operator[](size_t _i) { return (&x)[_i]; }
	constexpr T const& operator[](size_t _i) const { return (&x)[_i]; }
	T& at(size_t _i) { assert(_i < 2); return (&x)[_i] }
	constexpr T const& at(size_t _i) const { assert(_i < 2); return (&x)[_i] }

	constexpr Vector2() = default;
	constexpr Vector2(Vector2 const& _v) : x(_v.x), y(_v.y) {}
	constexpr explicit Vector2(T _s) : x(_s), y(_s) {}
	constexpr explicit Vector2(T _x, T _y) : x(_x), y(_y) {}
};

template<typename T> struct Vector4
{
	T x, y, z, w;
	T& operator[](size_t _i) { return (&x)[_i]; }
	constexpr T const& operator[](size_t _i) const { return (&x)[_i]; }
	T& at(size_t _i) { assert(_i < 4); return (&x)[_i]; }
	constexpr T const& at(size_t _i) const { assert(_i < 4); return (&x)[_i]; }

	constexpr Vector4() = default;
	constexpr Vector4(const Vector4& _v) : x(_v.x), y(_v.y), z(_v.z), w(_v.w) {}
	constexpr explicit Vector4(T _s) : x(_s), y(_s), z(_s), w(_s) {}
	constexpr explicit Vector4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
};

//=====================================================

template<typename T> struct Vector3
{
	T x, y, z;
	T& operator[](size_t _i) { return (&x)[_i]; }
	constexpr T const& operator[](size_t _i) const { return (&x)[_i]; }
	T& at(size_t _i) { assert(_i < 3); return (&x)[_i]; }
	constexpr T const& at(size_t _i) const { assert(_i < 3); return (&x)[_i]; }

	constexpr Vector3() = default;
	constexpr Vector3(const Vector3& _v) : x(_v.x), y(_v.y), z(_v.z) {}
	constexpr explicit Vector3(T _s) : x(_s), y(_s), z(_s) {}
	constexpr explicit Vector3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
};

template<typename T> constexpr Vector3<T> operator-(const Vector3<T>& v)
{
	return Vector3<T>(-v[0], -v[1], -v[2]);
}

template<typename T> constexpr Vector3<T> operator+(const Vector3<T>& a, const Vector3<T>& b)
{
	return Vector3<T>(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}

template<typename T> constexpr Vector3<T> operator*(const Vector3<T>& v, T const s)
{
	return Vector3<T>(v[0] * s, v[1] * s, v[2] * s);
}

template<typename T> constexpr Vector3<T> operator*(T const s, const Vector3<T>& v)
{
	return Vector3<T>(s * v[0], s * v[1], s * v[2]);
}

// entrywise multiplication
template<typename T> constexpr Vector3<T> operator*(const Vector3<T>& a, const Vector3<T>& b)
{
	return Vector3<T>(a[0] * b[0], a[1] * b[1], a[2] * b[2]);
}

template<typename T> constexpr Vector3<T> operator/(const Vector3<T>& v, T const s)
{
	return Vector3<T>(v[0] / s, v[1] / s, v[2] / s);
}

// entrywise division
template<typename T> constexpr Vector3<T> operator/(const Vector3<T>& a, const Vector3<T>& b)
{
	return Vector3(a[0] / b[0], a[1] / b[1], a[2] / b[2]);
}

template<typename T> constexpr bool NearlyEqual(T a, T b, T epsilon = std::numeric_limits<T>::epsilon())
{
	return std::abs(a - b) < epsilon;
}

template<typename T> constexpr T InnerProduct(const Vector3<T>& a, const Vector3<T>& b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

template<typename T> constexpr T L1Norm(const Vector3<T>& v)
{
	return std::abs(v[0]) + std::abs(v[1]) + std::abs(v[2]);
}

template<typename T> constexpr T L2Norm(const Vector3<T>& v)
{
	return std::sqrt(InnerProduct(v, v));
}

template<typename T> constexpr Vector3<T> L2Normalize(const Vector3<T>& v)
{
	return v / L2Norm(v);
}

template<typename T> constexpr Vector3<T> Cross(const Vector3<T>& a, const Vector3<T>& b)
{
	return Vector3<T>(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
}

//=================================================

template<typename T> struct Matrix3x3
{
	Vector3<T>	x, y, z;
	constexpr Matrix3x3() = default;
	constexpr Matrix3x3(const Matrix3x3& _m) : x(_m.x), y(_m.y), z(_m.z) {}
	constexpr explicit Matrix3x3(Vector3<T> const& _v0, Vector3<T> const& _v1, Vector3<T> const& _v2)
		: x(_v0), y(_v1), z(_v2) {}
	constexpr explicit Matrix3x3(T _00, T _01, T _02, T _10, T _11, T _12, T _20, T _21, T _22)
		: x(_00, _01, _02), y(_10, _11, _12), z(_20, _21, _22) {}

	Vector3<T>& operator[](size_t _i) { return (&x)[_i]; }
	constexpr Vector3<T> const& operator[](size_t _i) const { return (&x)[_i]; }
	Vector3<T>& at(size_t _i) { assert(_i < 3); return (&x)[_i]; }
	constexpr Vector3<T> const& at(size_t _i) const { assert(_i < 3); (&x)[_i]; }
};

template<typename T> constexpr T Determinant(const Matrix3x3<T>& m)
{
	return m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
		 - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
		 + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
}

template<typename T> /*constexpr*/ Matrix3x3<T> Inverse(const Matrix3x3<T>& m)
{
	T denom = static_cast<T>(1) / Determinant(m);
	return Matrix3x3<T>(
		  (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * denom,
		- (m[0][1] * m[2][2] - m[2][1] * m[0][2]) * denom,
		  (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * denom,
		- (m[1][0] * m[2][2] - m[2][0] * m[1][2]) * denom,
		  (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * denom,
		- (m[0][0] * m[1][2] - m[1][0] * m[0][2]) * denom,
		  (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * denom,
		- (m[0][0] * m[2][1] - m[2][0] * m[0][1]) * denom,
		  (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * denom);
}

template<typename T> constexpr Matrix3x3<T> operator*(const Matrix3x3<T>& a, const Matrix3x3<T>& b)
{
	return Matrix3x3<T>(
		a[0][0] * b[0][0] + a[1][0] * b[0][1] + a[2][0] * b[0][2],
		a[0][1] * b[0][0] + a[1][1] * b[0][1] + a[2][1] * b[0][2],
		a[0][2] * b[0][0] + a[1][2] * b[0][1] + a[2][2] * b[0][2],
		a[0][0] * b[1][0] + a[1][0] * b[1][1] + a[2][0] * b[1][2],
		a[0][1] * b[1][0] + a[1][1] * b[1][1] + a[2][1] * b[1][2],
		a[0][2] * b[1][0] + a[1][2] * b[1][1] + a[2][2] * b[1][2],
		a[0][0] * b[2][0] + a[1][0] * b[2][1] + a[2][0] * b[2][2],
		a[0][1] * b[2][0] + a[1][1] * b[2][1] + a[2][1] * b[2][2],
		a[0][2] * b[2][0] + a[1][2] * b[2][1] + a[2][2] * b[2][2]
		);
}

template<typename T> constexpr Vector3<T> operator*(const Matrix3x3<T>& m, const Vector3<T>& v)
{
	return Vector3<T>(
		m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2],
		m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2],
		m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2]
	);
}

template<typename T> constexpr Matrix3x3<T> operator*(T const& s, Matrix3x3<T> const& m)
{
	return Matrix3x3<T>(s * m[0], s * m[1], s* m[2]);
}

//=================================================

using Int2		= Vector2<int>;
using Float2	= Vector2<float>;
using Double2	= Vector2<double>;

using Int3		= Vector3<int>;
using Float3	= Vector3<float>;
using Double3	= Vector3<double>;

using Float3x3	= Matrix3x3<float>;

} // namespace math