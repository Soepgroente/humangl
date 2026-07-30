#pragma once
#include <cstdint>
#include <vector>

#include "Vectors.hpp"

namespace ve {

static_assert(sizeof(mat4) == 64 && "mat4 type size has to be 64B");
static_assert(sizeof(vec4) == 16 && "vec4 type size has to be 16B");

// [has to comply with std140]
class ViewProjectUniform
{
	public:
		ViewProjectUniform() = delete;
		ViewProjectUniform(const mat4& view, const mat4& projection, const mat4& orthographic) :
			view{view},
			projection{projection},
			orthographic{orthographic} {}

		void	updateView(const mat4& view) noexcept { this->view = view; }
		void	updateProjection(const mat4& prj) noexcept { this->projection = prj; }
		void	updateOrthographic(const mat4& ortho) noexcept { this->orthographic = ortho; }

		const void*	getData() const noexcept { return static_cast<const void*>(this); }

	private:
		mat4	view;
		mat4	projection;
		mat4	orthographic;
};
// check this: https://chatgpt.com/s/t_6a0c79f4930881919ea36656e039782a
static_assert((sizeof(ViewProjectUniform) % 16 == 0) && "type not ...");

// Phong lighting model, a combination of the following:
// Ambient lighting: even when it is dark there is usually still some light somewhere in the world
// 		(the moon, a distant light) so objects are almost never completely dark. To simulate this
// 		we use an ambient lighting constant that always gives the object some color.
// Diffuse lighting: simulates the directional impact a light object has on an object. This is the
// 		most visually significant component of the lighting model. The more a part of an object
// 		faces the light source, the brighter it becomes.
// Specular lighting: simulates the bright spot of a light that appears on shiny objects. Specular
// 		highlights are more inclined to the color of the light than the color of the object.
// [has to comply with std140]
struct MaterialData
{
	vec4	ambientColor;			// range [0-1] - indirect light color (darker than diffuse)
	vec4	diffuseColor;			// range [0-1] - color of the mesh
	vec4	specularColor;			// range [0-1] - reflex of the light
	f32		shininess;				// range [1.0-256.0] - low (2-8): opaque, high (64-256) shiny/metal
	f32		opacity;				// alpha of diffuse
    i32		refractionIndex{1};		// [not used yet, but keep it for padding] range [1-2.42...] - index of refraction, 1.0: air, 1.33 h2o, 1.5 glass								
	i32		illuminationModel{1};	// [not used yet, but keep it for padding] range [0-10] - 0: no lighting only texture, 1: ambient + diffuse, 2: ambient + diffuse + specular
};

// [has to comply with std140]
struct LightData
{
	vec4	lightAmbientColor;
	vec4	lightColor;
	vec4	lightSpecularColor;
	vec4 	lightDir;
};

struct VkConstants
{
	ui32	models{0U};
	ui32	materials{0U};
	ui32	lights{0U};
	ui32	fontColor{0U};
	ui32	textures{0U};
};

inline constexpr VkConstants drawingDataLimits{8U, 8U, 1U, 2U, 4U};

// [has to comply with std140]
class MeshUniform
{
	public:
		void	updateModelMatrix(ui32 index, const mat4& modelMatrix) noexcept;
		void	updateNormalMatrix(ui32 index, const mat4& normalMatrix) noexcept;
		void	updateMaterial(ui32 index, const MaterialData& newMaterial) noexcept;
		void	updateLight(ui32 index, const LightData& newLight, const mat4& viewMatrix) noexcept;
		void	updateLightDir(ui32 index, const vec3& newDir, const mat4& viewMatrix) noexcept;

		const void*	getData(void) const noexcept { return static_cast<const void*>(this); }

	private:
		mat4 			models[drawingDataLimits.models];
		mat4 			normals[drawingDataLimits.models];
		MaterialData 	materials[drawingDataLimits.materials];
		LightData 		lights[drawingDataLimits.lights];
};


// [has to comply with std140]
class TextUniform
{
	public:
		void	updateColor(ui32 index, const vec4& color) noexcept;

		const void*	getData() const noexcept { return static_cast<const void*>(this); }

	private:
		vec4	color[drawingDataLimits.fontColor];
};

class PushConstantsData {};

}	// namespace ve
