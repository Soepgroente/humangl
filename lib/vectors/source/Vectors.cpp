#include "Vectors.hpp"

f32	fastInverseSqrt(f32 number) noexcept
{
	FloatIntUnion	floatInt;
	
	floatInt.f = number;
	floatInt.i = 0x5f375a86 - (floatInt.i >> 1);
	floatInt.f = floatInt.f * (1.5f - (number * 0.5f * floatInt.f * floatInt.f));
	return floatInt.f;
}

f32	radians(f32 degrees) noexcept
{
	return degrees * (M_PI / 180.0f);
}

f32	radiansToDegrees(f32 radians) noexcept
{
	return radians * (180.0f / M_PI);
}
