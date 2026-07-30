#pragma once

#include "../type_aliases.hpp"
#include <random>
#include <chrono>
#include <vector>

#include "Vectors.hpp"

namespace ve {

template <typename T, typename... Rest>
void	hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
{
	seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	(hashCombine(seed, rest), ...);
};

f32		randomFloat(f32 min = 0.0f, f32 max = 1.0f);
i32		randomInt(i32 min = -500, i32 max = 500);
ui32	randomUint(ui32 min = 0U, ui32 max = 1000U);
vec3	generateRandomColor();
vec3	generateRandomGreyscale();
vec3	generateSoftGreyscale();

std::vector<unsigned char>	readFile(const std::string& filePath);

}	// namespace ve