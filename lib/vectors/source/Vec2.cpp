#include "Vec2.hpp"

#include <algorithm>

vec2&	vec2::operator=(const vec2& other)
{
	if (this != &other)
	{
		x = other.x;
		y = other.y;
	}
	return *this;
}

bool	vec2::operator<(const vec2& other) const noexcept
{
	if (x < other.x)
	{
		return true;
	}
	if (x > other.x)
	{
		return false;	
	}
	return y < other.y;
}

vec2&	vec2::normalize() noexcept
{
	f32 len = length();

	if (len != 0.0f)
	{
		*this /= len;
	}
	return *this;
}

vec2&	vec2::fastNormalize() noexcept
{
	f32 lenSq = lengthSquared();

	if (lenSq != 0.0f)
	{
		f32 invSqrt = fastInverseSqrt(lenSq);
		*this *= invSqrt;
	}
	return *this;
}

vec2&	vec2::rotate(f32 angleRadians) noexcept
{
	f32	cosAngle = std::cos(angleRadians);
	f32	sinAngle = std::sin(angleRadians);

	x = x * cosAngle - y * sinAngle;
	y = x * sinAngle + y * cosAngle;
	return *this;
}

vec2	vec2::rotated(f32 angleRadians) const noexcept
{
	f32	cosAngle = std::cos(angleRadians);
	f32	sinAngle = std::sin(angleRadians);

	return vec2(x * cosAngle - y * sinAngle, x * sinAngle + y * cosAngle);
}

/*	Careful, only insert normalized vectors when computing angles	*/

f32	vec2::angle(const vec2& a, const vec2& b) noexcept
{
	f32	dotProduct = vec2::dot(a, b);
	
	dotProduct = std::clamp(dotProduct, -1.0f, 1.0f);
	return std::acos(dotProduct);
}

f32	vec2::distanceSquared(const vec2& a, const vec2& b) noexcept
{
	f32	dx = b.x - a.x;
	f32	dy = b.y - a.y;

	return dx * dx + dy * dy;
}

vec2	vec2::lerp(const vec2& a, const vec2& b, f32 t) noexcept
{
	return vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

std::ostream&	operator<<(std::ostream& os, const vec2& v)
{
	os << "vec2(" << v.x << ", " << v.y << ")";
	return os;
}