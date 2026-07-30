#include "VulkanUtils.hpp"

#include <fstream>
#include <string>
#include <cassert>


namespace ve {

f32 randomFloat(f32 min, f32 max)
{
	assert(min < max && "Min value is bigger than max");
	static std::default_random_engine	engine(std::chrono::system_clock::now().time_since_epoch().count());
	static std::uniform_real_distribution<f32> distribution(0.0f, 1.0f);

	return distribution(engine) * (max - min) + min;
}

i32 randomInt(i32 min, i32 max)
{
	assert(min < max && "Min value is bigger than max");

	static std::default_random_engine engine(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<i32> distribution(min, max);
    return distribution(engine);
}

ui32 randomUint(ui32 min, ui32 max)
{
	assert( min < max && "Min value is bigger than max");

	static std::default_random_engine engine(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<ui32> distribution(min, max);
    return distribution(engine);
}

vec3	generateRandomColor()
{
	return vec3(randomFloat(), randomFloat(), randomFloat());
}

vec3	generateRandomGreyscale()
{
	f32	grey = randomFloat();

	return vec3(grey, grey, grey);
}

vec3	generateSoftGreyscale()
{
	static std::default_random_engine	engine(std::chrono::system_clock::now().time_since_epoch().count());
	static std::uniform_real_distribution<f32>	distribution(0.25f, 0.4f);
	f32	grey = distribution(engine);

	return vec3(grey, grey, grey);
}


std::vector<unsigned char> readFile(const std::string& filePath)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (file.is_open() == false)
	{
		throw std::runtime_error("failed to open file: " + filePath);
	}
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<unsigned char>	buffer(fileSize);

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}

}	// namespace ve