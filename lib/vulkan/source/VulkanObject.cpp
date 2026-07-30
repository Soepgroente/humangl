#include "VulkanObject.hpp"

#include <cassert>


namespace ve {

ui32	VulkanObject::currentID = 0U;

void	VulkanObject::rotate(const vec3& axis, f32 angle) noexcept
{
	this->transformationApplied = true;
	this->rotation = quat::product(quat(angle, axis), this->rotation);	// second factor is applied first, first as last
	this->rotation.normalize();
}

void	VulkanObject::translate(const vec3& translation) noexcept
{
	this->transformationApplied = true;
	this->translation += translation;
}

void	VulkanObject::scale(const vec3& scalar) noexcept
{
	this->transformationApplied = true;
	if (this->uniformScale == true and (scalar.x != scalar.y or scalar.y != scalar.z))
	{
		this->uniformScale = false;
	}

	this->scalar.x *= scalar.x;
	this->scalar.y *= scalar.y;
	this->scalar.z *= scalar.z;
}

void	VulkanObject::scale(f32 scalar) noexcept
{
	this->transformationApplied = true;
	this->scalar *= scalar;
}

void	VulkanObject::bindBuffer(VkCommandBuffer commandBuffer) const noexcept
{
	assert(this->model != nullptr && "Model not set");
	this->model->bindBuffer(commandBuffer);
};

void	VulkanObject::draw(VkCommandBuffer commandBuffer) const noexcept
{
	assert(this->model != nullptr && "Model not set");
	this->model->draw(commandBuffer);
};

std::shared_ptr<VulkanModel>	VulkanObject::getModel() const noexcept
{
	assert(this->model != nullptr && "Model not set");
	return this->model;
};

mat4	VulkanObject::getModelMatrix(bool columnMajor) const noexcept
{
	if (this->transformationApplied == false)
	{
		return mat4::idMat();
	}

	// model = T × R × S
	mat4 rotMatrix = this->rotation.getMatrix();

	if (columnMajor == true)
	{
		return mat4(
			{this->scalar.x * rotMatrix[0][0],  this->scalar.x * rotMatrix[0][1],  this->scalar.x * rotMatrix[0][2],  0.0f},
			{this->scalar.y * rotMatrix[1][0],  this->scalar.y * rotMatrix[1][1],  this->scalar.y * rotMatrix[1][2],  0.0f},
			{this->scalar.z * rotMatrix[2][0],  this->scalar.z * rotMatrix[2][1],  this->scalar.z * rotMatrix[2][2],  0.0f},
			{this->translation.x,             this->translation.y,             this->translation.z,             1.0f}
		);
	}
	else
	{
		return mat4(
			{this->scalar.x * rotMatrix[0][0],  this->scalar.y * rotMatrix[1][0],  this->scalar.z * rotMatrix[2][0],  this->translation.x},
			{this->scalar.x * rotMatrix[0][1],  this->scalar.y * rotMatrix[1][1],  this->scalar.z * rotMatrix[2][1],  this->translation.y},
			{this->scalar.x * rotMatrix[0][2],  this->scalar.y * rotMatrix[1][2],  this->scalar.z * rotMatrix[2][2],  this->translation.z},
			{0.0f,                                      0.0f,                                      0.0f,                                      1.0f}
		);
	}
}

mat4	VulkanObject::getNormalMatrix(bool columnMajor) const noexcept
{
	// normalMatrix = (Model⁻¹)ᵀ = ((T × R × S)⁻¹)ᵀ = (S⁻¹ * R⁻¹ * T⁻¹)ᵀ = (T⁻¹)ᵀ * (R⁻¹)ᵀ * (S⁻¹)ᵀ but:
	// Transpose matrix: since it moves points, doesn't affect the normal so can be removed
	// Rotation matrix: is orthogonal so  R⁻¹ = Rᵀ --> (R⁻¹)ᵀ = (Rᵀ)ᵀ = R
	// Scale matrix: is diagonal (and therefore symmetric) so Sᵀ = S --> (S⁻¹)ᵀ = S⁻¹ also the inverse of a diagonal matrix
	//		is a matrix which elements are the inverse of the elements of the original matrix, finally, if the scaling is uniform
	//		i.e scale.x = scale.y = scale.z the whole matrix can be ignored
	// therefore: (Model⁻¹)ᵀ = R * [S⁻¹]
	if (this->transformationApplied == false)
	{
		return mat4::idMat();
	}

	mat4 normal = this->rotation.getMatrix();

	if (this->uniformScale == false)
	{
		const f32 idx = 1.0f / this->scalar.x;
		const f32 idy = 1.0f / this->scalar.y;
		const f32 idz = 1.0f / this->scalar.z;

		normal[0][0] *= idx;
		normal[0][1] *= idx;
		normal[0][2] *= idx;
		normal[1][0] *= idy;
		normal[1][1] *= idy;
		normal[1][2] *= idy;
		normal[2][0] *= idz;
		normal[2][1] *= idz;
		normal[2][2] *= idz;
	}

	if (columnMajor == true)
	{
		return normal;
	}
	else
	{
		return normal.transpose();
	}
}

mat4	VulkanObject::getNormalViewMatrix(const mat4& viewNoTranslation, bool columnMajor) const noexcept
{
	// normalMatrix = ((View * Model)⁻¹)ᵀ = ((Model)⁻¹ * (View)⁻¹)ᵀ = (View⁻¹)ᵀ * (Model⁻¹)ᵀ = (View⁻¹)ᵀ * R * [S⁻¹] ( --> see getNormalMatrix() ) but:
	// ViewMatrix: if it has no translation it has only rotation features, so is again orthogonal
	//		so  View⁻¹ = Viewᵀ --> (View⁻¹)ᵀ = (Viewᵀ)ᵀ = View
	// therefore: normalMatrix = ViewNoTrans * R * [S⁻¹]
	if (this->transformationApplied == false)
	{
		return viewNoTranslation;
	}

	mat4 normalView = this->getNormalMatrix(columnMajor) * viewNoTranslation;
	if (columnMajor == true)
	{
		return normalView;
	}
	else
	{
		return normalView.transpose();
	}
}

MeshLayoutDescription	VulkanObject::getModelLayout() const noexcept
{
	assert(this->model != nullptr && "Model not set");
	return this->model->getModelLayout();
}

}	// namespace ve