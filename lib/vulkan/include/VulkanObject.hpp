#pragma once

#include "../type_aliases.hpp"
#include "Vectors.hpp"
#include "VulkanModel.hpp"
#include "VulkanUniform.hpp"

#include <memory>
#include <map>
#include <string>
#include <unordered_map>


namespace ve {

struct Material
{
	std::string	name;
	vec4	ambientClr;
	vec4	diffuseClr;
	vec4	specularClr;
	i32		shininess;
	f32		opacity;
	i32		refractionIndex;
	i32		illuminationModel;
	bool	smoothShading;
};

struct ObjComponent
{
	std::vector<std::vector<ui32>>	faceIndices;
	std::vector<std::vector<ui32>>	textureIndices;
	std::vector<std::vector<ui32>>	normalIndices;
	std::string						matName;
};

struct ObjInfo
{
	std::string						name;
	std::string						mtlFile;
	std::vector<vec3>				vertices;
	std::vector<vec2>				textureCoords;
	std::vector<vec3>				normals;
	std::vector<vec3>				colors;
	std::vector<ObjComponent>		components;
	std::map<std::string, Material>	materials;
};

class VulkanObject
{
	public:
		VulkanObject() : id(currentID++) {};
		VulkanObject(const VulkanObject& other) = delete;
		VulkanObject(VulkanObject&& other) = default;
		VulkanObject& operator=(const VulkanObject& other) = delete;
		VulkanObject& operator=(VulkanObject&& other) = delete;
		~VulkanObject() = default;

		void	rotate(const vec3& axis, f32 angle) noexcept;
		void	translate(const vec3& translation) noexcept;
		void	scale(const vec3& scalar) noexcept;
		void	scale(f32 scalar) noexcept;
		
		void	bindBuffer(VkCommandBuffer commandBuffer) const noexcept;
		void	draw(VkCommandBuffer commandBuffer) const noexcept;
		
		void							setModel(std::shared_ptr<VulkanModel> newModel) noexcept { this->model = newModel; };
		std::shared_ptr<VulkanModel>	getModel() const noexcept;
		void							setMaterial(MaterialData const& material) noexcept { this->materialData = material; };
		MaterialData const&				getMaterial() const noexcept { return this->materialData; };
		ui32							getID() const noexcept { return this->id; }
		MeshLayoutDescription			getModelLayout() const noexcept;

		mat4							getModelMatrix(bool columnMajor = false) const noexcept;
		mat4							getNormalMatrix(bool columnMajor = false) const noexcept;
		mat4							getNormalViewMatrix(const mat4& viewNoTranslation, bool columnMajor = false) const noexcept;

	private:
		ui32							id;
		std::shared_ptr<VulkanModel>	model{nullptr};
		MaterialData					materialData{};
		
		vec3	translation{0.0f};
		vec3	scalar{1.0f, 1.0f, 1.0f};
		quat	rotation{};

		bool	transformationApplied{false};
		bool	uniformScale{true};

		static ui32		currentID;
};

std::ostream&	operator<<(std::ostream& os, const ObjInfo& obj);
std::vector<ObjInfo>	parseOBJFile(const std::string& objFilePath);

} // namespace ve
