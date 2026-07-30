#pragma once

#include <array>
#include <ostream>
#include "../type_aliases.hpp"

class vec3;

class mat4;

class mat3
{
	public:

	static mat3	idMat( void );

	mat3() = default;
	mat3(f32 diagonal);
	mat3(const vec3& row0,
		 const vec3& row1,
		 const vec3& row2);
	mat3(const mat4& matrix4x4);
	mat3(std::initializer_list<std::initializer_list<f32>> rows);
	~mat3() = default;
	mat3(const mat3& other) = default;
	mat3&	operator=(const mat3& other) = default;

	mat3	operator*(const mat3& other) const;
	mat3&	operator*=(const mat3& other) {	*this = *this * other; return *this;}

	f32*			operator[](i32 row) noexcept { return data[row]; }
	const f32*	operator[](i32 row) const noexcept { return data[row]; }

	f32	data[3][3];

	private:
};

std::ostream&	operator<<(std::ostream& os, const mat3& matrix);