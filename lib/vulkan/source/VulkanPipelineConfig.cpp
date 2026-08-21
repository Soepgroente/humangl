#include "VulkanPipeline.hpp"

#include <cassert>

namespace ve {

static inline VkPipelineViewportStateCreateInfo	getVkPipelineViewportStateCreateInfo()
{
	return VkPipelineViewportStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = nullptr,
		.scissorCount = 1,
		.pScissors = nullptr
	};
}

static inline VkPipelineRasterizationStateCreateInfo	getVkPipelineRasterizationStateCreateInfo(TextureType tex)
{
	VkPipelineRasterizationStateCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f
	};
	if (tex == TEXTURE_FONT || tex == TEXTURE_CUBEMAP)
	{
		info.cullMode = VK_CULL_MODE_NONE;
	}
	return info;
}

static inline VkPipelineInputAssemblyStateCreateInfo	getVkPipelineInputAssemblyStateCreateInfo()
{
	return VkPipelineInputAssemblyStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};
}

static inline VkPipelineMultisampleStateCreateInfo	getVkPipelineMultisampleStateCreateInfo()
{
	return VkPipelineMultisampleStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};
}

static inline VkPipelineColorBlendAttachmentState	getVkPipelineColorBlendAttachmentState(TextureType tex)
{
	VkPipelineColorBlendAttachmentState	state {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD
	};
	if (tex == TEXTURE_FONT)
	{
		state.blendEnable = VK_TRUE;
		state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	}
	return state;
}

static inline VkPipelineColorBlendStateCreateInfo	getVkPipelineColorBlendStateCreateInfo(VkPipelineColorBlendAttachmentState* attachment)
{
	return VkPipelineColorBlendStateCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = attachment,
		.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
	};
}

static inline VkPipelineDepthStencilStateCreateInfo	getVkPipelineDepthStencilStateCreateInfo(TextureType tex)
{
	VkPipelineDepthStencilStateCreateInfo info {

		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
		.stencilTestEnable = VK_FALSE,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS
	};
	if (tex == TEXTURE_CUBEMAP)
	{
		info.depthTestEnable = VK_TRUE;
		info.depthWriteEnable = VK_FALSE;
		info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	}
	else if (tex == TEXTURE_FONT)
	{
		info.depthTestEnable  = VK_FALSE;
		info.depthWriteEnable = VK_FALSE;
		info.depthCompareOp   = VK_COMPARE_OP_ALWAYS;
	}
	return info;
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
		const ui32 nEntries = 5;

		configInfo.constantEntries = std::make_unique<VkSpecializationMapEntry[]>(nEntries);
		for (ui32 i = 0; i < nEntries; i++)
		{
			configInfo.constantEntries[i] = VkSpecializationMapEntry {
				.constantID = i,
				.size = sizeof(ui32)
			};			
		}
		configInfo.constantEntries[0].offset = offsetof(VkConstants, models);
		configInfo.constantEntries[1].offset = offsetof(VkConstants, materials);
		configInfo.constantEntries[2].offset = offsetof(VkConstants, lights);
		configInfo.constantEntries[3].offset = offsetof(VkConstants, fontColor);
		configInfo.constantEntries[4].offset = offsetof(VkConstants, textures);

		configInfo.constantsInfo = std::make_unique<VkSpecializationInfo>();
		configInfo.constantsInfo->mapEntryCount = nEntries;
		configInfo.constantsInfo->pMapEntries = configInfo.constantEntries.get();
		configInfo.constantsInfo->dataSize = sizeof(VkConstants);
		configInfo.constantsInfo->pData = constants;
	}
	configInfo.shadersConfig.resize(shaders.size());
	for (ui32 i = 0; i < shaders.size(); i++)
	{
		configInfo.shadersConfig[i] = VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = shaders[i].getStageFlag(),
			.module = shaders[i].getModule(),
			.pName = "main",
			.pSpecializationInfo = configInfo.constantsInfo.get()
		};
	}

	configInfo.viewportInfo = getVkPipelineViewportStateCreateInfo();
	configInfo.inputAssemblyInfo = getVkPipelineInputAssemblyStateCreateInfo();
	configInfo.rasterizationInfo = getVkPipelineRasterizationStateCreateInfo(textureUsed);
	configInfo.multisampleInfo = getVkPipelineMultisampleStateCreateInfo();
	configInfo.colorBlendAttachment = getVkPipelineColorBlendAttachmentState(textureUsed);
	configInfo.colorBlendInfo = getVkPipelineColorBlendStateCreateInfo(&configInfo.colorBlendAttachment);
	configInfo.depthStencilInfo = getVkPipelineDepthStencilStateCreateInfo(textureUsed);

	configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	configInfo.dynamicStateInfo.flags = 0;
	configInfo.dynamicStateInfo.dynamicStateCount = static_cast<ui32>(configInfo.dynamicStateEnables.size());
	configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();

	return configInfo;
}

}	// namespace ve