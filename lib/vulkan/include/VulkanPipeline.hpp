#pragma once

#include "VulkanDevice.hpp"
#include "VulkanModel.hpp"
#include "VulkanShader.hpp"
#include "VulkanTexture.hpp"
#include "VulkanUniform.hpp"

#include <vector>

namespace ve {

struct PipelineConfig
{
	VkPipelineVertexInputStateCreateInfo 		vertexInputInfo;
	VkPipelineViewportStateCreateInfo			viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo		inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo		rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo		multisampleInfo;
	VkPipelineColorBlendAttachmentState			colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo			colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo		depthStencilInfo;
	std::vector<VkDynamicState>					dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo			dynamicStateInfo;
	std::shared_ptr<VkSpecializationInfo>		constantsInfo;
	std::shared_ptr<VkSpecializationMapEntry[]>	constantEntries;

	std::vector<VkVertexInputBindingDescription>	bindingVboConfig;
	std::vector<VkVertexInputAttributeDescription>	attributeVboConfig;
	std::vector<VkPipelineShaderStageCreateInfo>	shadersConfig;
};

class VulkanPipeline
{
	public:

	VulkanPipeline() = delete;
	VulkanPipeline(
		VulkanDevice& device,
		const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
		VkRenderPass renderPass,
		const std::string& vertexShaderFile,
		const std::string& fragmentShaderFile,
		const MeshLayoutDescription& meshLayout,
		TextureType textureUsed,
		ui32 sizePushConstants,
		const VkConstants* constants
	);
	~VulkanPipeline();
	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline(VulkanPipeline&&) noexcept;
	VulkanPipeline& operator=(const VulkanPipeline&) = delete;

	VkPipelineLayout	getPipelineLayout() const noexcept { return pipelineLayout; };
	void				bindPipeline(VkCommandBuffer commandBuffer) const noexcept;
	void				updatePushConstants(VkCommandBuffer commandBuffer, const void* data) const noexcept;

	static std::unique_ptr<VulkanPipeline> createPipeline(
		VulkanDevice& device,
		std::vector<VkDescriptorSetLayout> const& descriptorSetLayouts,
		VkRenderPass renderPass,
		const std::string& vertexShaderFile,
		const std::string& fragmentShaderFile,
		const MeshLayoutDescription& meshLayout,
		TextureType textureUsed = TEXTURE_PLAIN,
		ui32 sizePushConstants = 0U,
		const VkConstants* constants = nullptr
	);

	private:

	void			setupPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts );
	void			setupPipeline(const std::string& vertexShaderFile, const std::string& fragmentShaderFile, const MeshLayoutDescription& meshLayout, TextureType textureUsed, VkRenderPass renderPass, const VkConstants* constants);
	PipelineConfig	getPipelineConfig(const std::vector<VulkanShader>& shaders, const MeshLayoutDescription& meshLayout, const VkConstants* constants, TextureType textureUsed) const noexcept;

	VulkanDevice&		vulkanDevice;
	VkPipelineLayout	pipelineLayout;
	VkPipeline			pipeline;

	ui32			sizePushConstants;
};

}