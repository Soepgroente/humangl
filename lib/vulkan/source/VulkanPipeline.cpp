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
		.layout = this->pipelineLayout,
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

PipelineConfig	VulkanPipeline::getPipelineConfig(
	const std::vector<VulkanShader>& shaders,
	const MeshLayoutDescription& meshLayout,
	const VkConstants* constants,
	TextureType textureUsed) const noexcept
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfig configInfo{};

	configInfo.bindingVboConfig = meshLayout.bindingConfig;
	configInfo.attributeVboConfig = meshLayout.attributeConfig;
	configInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	configInfo.vertexInputInfo.pNext = nullptr;
	configInfo.vertexInputInfo.flags = 0U;
	configInfo.vertexInputInfo.vertexBindingDescriptionCount = configInfo.bindingVboConfig.size();
	configInfo.vertexInputInfo.pVertexBindingDescriptions = configInfo.bindingVboConfig.data();
	configInfo.vertexInputInfo.vertexAttributeDescriptionCount = configInfo.attributeVboConfig.size();
	configInfo.vertexInputInfo.pVertexAttributeDescriptions = configInfo.attributeVboConfig.data();

	if (constants != nullptr)
	{
		ui32 nEntries = 5;
		configInfo.constantEntries = std::make_unique<VkSpecializationMapEntry[]>(nEntries);
		configInfo.constantEntries[0].constantID = 0;
		configInfo.constantEntries[0].offset = offsetof(VkConstants, models);
		configInfo.constantEntries[0].size = sizeof(ui32);

		configInfo.constantEntries[1].constantID = 1;
		configInfo.constantEntries[1].offset = offsetof(VkConstants, materials);
		configInfo.constantEntries[1].size = sizeof(ui32);

		configInfo.constantEntries[2].constantID = 2;
		configInfo.constantEntries[2].offset = offsetof(VkConstants, lights);
		configInfo.constantEntries[2].size = sizeof(ui32);

		configInfo.constantEntries[3].constantID = 3;
		configInfo.constantEntries[3].offset = offsetof(VkConstants, fontColor);
		configInfo.constantEntries[3].size = sizeof(ui32);

		configInfo.constantEntries[4].constantID = 4;
		configInfo.constantEntries[4].offset = offsetof(VkConstants, textures);
		configInfo.constantEntries[4].size = sizeof(ui32);

		configInfo.constantsInfo = std::make_unique<VkSpecializationInfo>();
		configInfo.constantsInfo->mapEntryCount = nEntries;
		configInfo.constantsInfo->pMapEntries = configInfo.constantEntries.get();
		configInfo.constantsInfo->dataSize = sizeof(VkConstants);
		configInfo.constantsInfo->pData = constants;
	}
	configInfo.shadersConfig.resize(shaders.size());
	for (ui32 i = 0; i < shaders.size(); i++)
	{
		configInfo.shadersConfig[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		configInfo.shadersConfig[i].pNext = nullptr;
		configInfo.shadersConfig[i].flags = 0;
		configInfo.shadersConfig[i].stage = shaders[i].getStageFlag();
		configInfo.shadersConfig[i].module = shaders[i].getModule();
		configInfo.shadersConfig[i].pName = "main";
		configInfo.shadersConfig[i].pSpecializationInfo = configInfo.constantsInfo.get();
	}

	configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	configInfo.viewportInfo.viewportCount = 1;
	configInfo.viewportInfo.pViewports = nullptr;
	configInfo.viewportInfo.scissorCount = 1;
	configInfo.viewportInfo.pScissors = nullptr;

	configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
	configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	configInfo.rasterizationInfo.lineWidth = 1.0f;

	if (textureUsed == TEXTURE_FONT || textureUsed == TEXTURE_CUBEMAP)
	{
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	}
	else
	{
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	}
	configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
	configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
	configInfo.rasterizationInfo.depthBiasClamp = 0.0f;
	configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

	configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
	configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	configInfo.multisampleInfo.minSampleShading = 1.0f;
	configInfo.multisampleInfo.pSampleMask = nullptr;
	configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

	configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	if (textureUsed == TEXTURE_FONT)
	{
		configInfo.colorBlendAttachment.blendEnable = VK_TRUE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	}
	else
	{
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	}
	configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
	configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	configInfo.colorBlendInfo.attachmentCount = 1;
	configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
	configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
	configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

	configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	configInfo.depthStencilInfo.minDepthBounds = 0.0f;
	configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
	configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;

	if (textureUsed == TEXTURE_CUBEMAP)
	{
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	}
	else if (textureUsed == TEXTURE_FONT)
	{
		configInfo.depthStencilInfo.depthTestEnable  = VK_FALSE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
		configInfo.depthStencilInfo.depthCompareOp   = VK_COMPARE_OP_ALWAYS;
	}
	else
	{
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	}

	configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	configInfo.dynamicStateInfo.flags = 0;
	configInfo.dynamicStateInfo.dynamicStateCount = static_cast<ui32>(configInfo.dynamicStateEnables.size());
	configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();

	return configInfo;
}



} // namespace ve
