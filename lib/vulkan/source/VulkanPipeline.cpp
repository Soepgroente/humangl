#include "VulkanPipeline.hpp"
#include "VulkanUtils.hpp"

#include <cassert>

namespace ve {


VulkanPipeline::VulkanPipeline(
	VulkanDevice& device,
	const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
	VkRenderPass renderPass,
	const std::string& vertexShaderFile,
	const std::string& fragmentShaderFile,
	const MeshLayoutDescription& meshLayout,
	TextureType textureUsed,
	ui32 sizePushConstants,
	const VkConstants* constants
) :
vulkanDevice{device}, sizePushConstants{sizePushConstants}
{
	setupPipelineLayout(descriptorSetLayouts);
	setupPipeline(vertexShaderFile, fragmentShaderFile, meshLayout, textureUsed, renderPass, constants);
}

VulkanPipeline::~VulkanPipeline()
{
	vkDestroyPipeline(vulkanDevice.device(), pipeline, nullptr);
	vkDestroyPipelineLayout(vulkanDevice.device(), pipelineLayout, nullptr);
}

std::unique_ptr<VulkanPipeline>	VulkanPipeline::createPipeline(
	VulkanDevice& device,
	const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
	VkRenderPass renderPass,
	const std::string& vertexShaderFile,
	const std::string& fragmentShaderFile,
	const MeshLayoutDescription& meshLayout,
	TextureType textureUsed,
	ui32 sizePushConstants,
	const VkConstants* constants
)
{
	// vulkan push_constant max size must be 128 or 256 b
	if (descriptorSetLayouts.size() == 0U)
	{
		throw std::runtime_error("no descritptor layour provided");
	}
	else if (sizePushConstants > device.getMaxSizePushConstants())
	{
		throw std::runtime_error("vulkan push_constant input size exceeds device limit");
	}

	return std::make_unique<VulkanPipeline>(
		device,
		descriptorSetLayouts,
		renderPass,
		vertexShaderFile,
		fragmentShaderFile,
		meshLayout,
		textureUsed,
		sizePushConstants,
		constants
	);
}

VulkanPipeline::VulkanPipeline(VulkanPipeline&& other) noexcept :
	vulkanDevice{other.vulkanDevice}
{
	if (this != &other)
	{
		pipelineLayout = other.pipelineLayout;
		pipeline = other.pipeline;
		sizePushConstants = other.sizePushConstants;
		other.pipelineLayout = VK_NULL_HANDLE;
		other.pipeline = VK_NULL_HANDLE;
	}
}

void	VulkanPipeline::bindPipeline(VkCommandBuffer commandBuffer) const noexcept
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
}

void	VulkanPipeline::updatePushConstants(VkCommandBuffer commandBuffer, const void* data ) const noexcept
{
	assert(data != nullptr && "Data source null");

	vkCmdPushConstants(
		commandBuffer,
		this->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		this->sizePushConstants,
		data
	);
}

void	VulkanPipeline::setupPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	VkPipelineLayoutCreateInfo	pipelineLayoutInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<ui32>(descriptorSetLayouts.size()),
		.pSetLayouts = descriptorSetLayouts.data()
	};

	if (this->sizePushConstants > 0U)
	{
		VkPushConstantRange pushRange
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = this->sizePushConstants
		};
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushRange;
	}
	errorCheck(vkCreatePipelineLayout(this->vulkanDevice.device(), &pipelineLayoutInfo, nullptr, &this->pipelineLayout), "failed to create pipeline layout!");
}

void	VulkanPipeline::setupPipeline(
	const std::string& vertexShaderFile,
	const std::string& fragmentShaderFile,
	const MeshLayoutDescription& meshLayout,
	TextureType textureUsed,
	VkRenderPass renderPass,
	const VkConstants* constants)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	std::vector<ve::VulkanShader> shaders;
	shaders.emplace_back(vulkanDevice, VK_SHADER_STAGE_VERTEX_BIT, vertexShaderFile);
	shaders.emplace_back(vulkanDevice, VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderFile);

	PipelineConfig pipelineConfig = getPipelineConfig(shaders, meshLayout, constants, textureUsed);

	VkPipelineRenderingCreateInfo renderInfo {
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = vulkanDevice.getSwapChainSupport().capabilities
	};

	VkGraphicsPipelineCreateInfo pipelineInfo
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = pipelineConfig.shadersConfig.size(),
		.pStages = pipelineConfig.shadersConfig.data(),
		.pVertexInputState = &pipelineConfig.vertexInputInfo,
		.pInputAssemblyState = &pipelineConfig.inputAssemblyInfo,
		.pViewportState = &pipelineConfig.viewportInfo,
		.pRasterizationState = &pipelineConfig.rasterizationInfo,
		.pMultisampleState = &pipelineConfig.multisampleInfo,
		.pColorBlendState = &pipelineConfig.colorBlendInfo,
		.pDepthStencilState = &pipelineConfig.depthStencilInfo,
		.pDynamicState = &pipelineConfig.dynamicStateInfo,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineIndex = -1,
		.basePipelineHandle = VK_NULL_HANDLE
	};

	if (vkCreateGraphicsPipelines(
		vulkanDevice.device(),
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&this->pipeline) != VK_SUCCESS)
	{
		vkDestroyPipelineLayout(this->vulkanDevice.device(), this->pipelineLayout, nullptr);
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

} // namespace ve
