#include "Vec4ui.hpp"

#include <cmath>

vec4ui&	vec4ui::operator=(const vec4ui& other)
{
	if (this != &other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
	}
	return *this;
}