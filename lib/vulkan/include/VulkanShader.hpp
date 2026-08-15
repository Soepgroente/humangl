#pragma once

#include "VulkanDevice.hpp"

namespace ve {

class VulkanShader
{
	public:
		VulkanShader() = delete;
		VulkanShader(VulkanDevice& device, VkShaderStageFlagBits shaderStageFlag, const std::string& shaderPath);
		~VulkanShader() noexcept;
		VulkanShader(const VulkanShader& other) = delete;
		VulkanShader(VulkanShader&& other) noexcept;
		VulkanShader& operator=(const VulkanShader& other) = delete;
		VulkanShader& operator=(VulkanShader&& other) = delete;

		VkShaderModule			getModule() const noexcept { return this->shaderModule; };
		VkShaderStageFlagBits	getStageFlag() const noexcept { return this->shaderStageFlag; };

	private:
		void	createModule(const std::vector<unsigned char>& fileContent);

		VulkanDevice&			vulkanDevice;
		VkShaderStageFlagBits	shaderStageFlag;
		VkShaderModule			shaderModule;
};

}	// namespace ve