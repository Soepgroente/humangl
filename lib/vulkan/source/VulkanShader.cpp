
#include "VulkanShader.hpp"
#include "VulkanUtils.hpp"

namespace ve {

VulkanShader::VulkanShader(VulkanDevice& device, VkShaderStageFlagBits shaderStageFlag, const std::string& shaderPath) : 
	vulkanDevice{device}, shaderStageFlag{shaderStageFlag}, shaderModule{VK_NULL_HANDLE}
{
	std::vector<unsigned char> content = readFile(shaderPath);
	this->createModule(content);
}

VulkanShader::VulkanShader(VulkanShader&& other) noexcept :
	vulkanDevice{other.vulkanDevice}
{
	if (this != &other)
	{
		this->shaderStageFlag = other.shaderStageFlag;
		this->shaderModule = other.shaderModule;
		other.shaderModule = VK_NULL_HANDLE;
	}
}

VulkanShader::~VulkanShader() noexcept
{
	vkDestroyShaderModule(this->vulkanDevice.device(), this->shaderModule, nullptr);
}

void	VulkanShader::createModule(const std::vector<unsigned char>& fileContent)
{
	VkShaderModuleCreateInfo createInfo
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = fileContent.size(),
		.pCode = reinterpret_cast<const ui32*>(fileContent.data())
	};

	errorCheck(vkCreateShaderModule(
			vulkanDevice.device(),
			&createInfo,
			nullptr,
			&shaderModule), "couldn't create shader module");
}

}	// namespace ve