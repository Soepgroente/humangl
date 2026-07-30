#pragma once

#include "Vec3.hpp"
#include "Mat4.hpp"
#include "Mat3.hpp"

#include <algorithm>
#include <ostream>
#include "../type_aliases.hpp"

class vec3;

class quat
{
	public:

	union
	{
		// #ifdef USE_SIMD
		// __m128	simdData;
		// #endif
		f32	data[4];
		struct
		{
			f32	w;
			f32	x;
			f32	y;
			f32	z;
		};
		struct
		{
			f32	scalar;
			f32	i;
			f32	j;
			f32	k;
		};
	};

	quat() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
	quat(f32 w, f32 x, f32 y, f32 z) : w(w), x(x), y(y), z(z) {}
	quat(f32 angle, const vec3& v3);
	quat(const quat& other) : w(other.w), x(other.x), y(other.y), z(other.z) {}
	quat&	operator=(const quat& other);
	~quat() = default;

	f32&			operator[](i32 index) noexcept { return data[index]; }
	const f32&	operator[](i32 index) const noexcept { return data[index]; }

	vec3	vector() const noexcept;
	quat	clone() const noexcept;
	quat&	conjugate() noexcept;
	quat	conjugated() const noexcept;

	quat&	normalize() noexcept;
	quat	normalized() const noexcept;
	quat&	fastNormalize() noexcept;
	quat	fastNormalized() const noexcept;

	mat4	getMatrix(bool columnMajor = true) const noexcept;

	static vec3 rotated(const vec3& rotateAround, quat rotation) noexcept;
	static quat product(const quat& a, const quat& b) noexcept;
};

std::ostream&	operator<<(std::ostream& os, const quat& q) noexcept;