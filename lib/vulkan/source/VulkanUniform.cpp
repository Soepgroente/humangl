#include "VulkanUniform.hpp"
#include <cassert>

namespace ve {

void	MeshUniform::updateModelMatrix(ui32 index, const mat4& modelMatrix) noexcept
{
	assert(index < drawingDataLimits.models && "model matrix index out of bounds");

	this->models[index] = modelMatrix;
}

void	MeshUniform::updateNormalMatrix(ui32 index, const mat4& normalMatrix) noexcept
{
	assert(index < drawingDataLimits.models && "normal matrix index out of bounds");

	this->normals[index] = normalMatrix;
}

void	MeshUniform::updateMaterial(ui32 index, const MaterialData& newMaterial) noexcept
{
	assert(index < drawingDataLimits.materials && "material index out of bounds");

	this->materials[index] = newMaterial;
}

void	MeshUniform::updateLight(ui32 index, const LightData& newLight, const mat4& viewMatrix) noexcept
{
	assert(index < drawingDataLimits.lights && "light source index out of bounds");

	this->lights[index] = newLight;
	this->updateLightDir(index, vec3{newLight.lightDir}, viewMatrix);
}

void	MeshUniform::updateLightDir(ui32 index, const vec3& newDir, const mat4& viewMatrix) noexcept
{
	assert(index < drawingDataLimits.lights && "light source index out of bounds");

	// light direction is in view space to avoid passing camera position to shaders
	vec4 reverseLightDir = vec4{newDir * -1, 0.0f};
	vec4 viewLightDir = (viewMatrix * reverseLightDir).normalize();

	this->lights[index].lightDir = viewLightDir;
}


void	TextUniform::updateColor(ui32 index, const vec4& color) noexcept
{
	assert(index < drawingDataLimits.fontColor && "text color index out of bounds");

	this->color[index] = color;
}

}	// namespace ve
