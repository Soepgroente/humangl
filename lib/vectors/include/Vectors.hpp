#pragma once

#include "Vec2.hpp"
#include "Vec2i.hpp"
#include "Vec2ui.hpp"
#include "Vec3.hpp"
#include "Vec3i.hpp"
#include "Vec3ui.hpp"
#include "Vec4.hpp"
#include "Vec4ui.hpp"
#include "Quat.hpp"
#include "Mat3.hpp"
#include "Mat4.hpp"

#include <cmath>
#include <limits>

constexpr f32	pi() noexcept { return 3.14159265358979323846f; }
constexpr f32	two_pi() noexcept { return 2.0f * pi(); }
constexpr f32	half_pi() noexcept { return pi() / 2.0f; }
constexpr f32	epsilon() noexcept { return std::numeric_limits<float>::epsilon(); }

union FloatIntUnion
{
	f32	f;
	int32_t	i;
};

f32	fastInverseSqrt(f32 number) noexcept;
f32	radians(f32 degrees) noexcept;
f32	radiansToDegrees(f32 radians) noexcept;