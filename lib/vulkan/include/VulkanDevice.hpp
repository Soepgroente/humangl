#pragma once

#include "VulkanWindow.hpp"
#include "../type_aliases.hpp"

// std lib headers
#include <string>
#include <vector>

namespace ve {

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
	ui32 graphicsFamily;
	ui32 presentFamily;
	bool graphicsFamilyHasValue = false;
	bool presentFamilyHasValue = false;
	bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

enum TextureType
{
	TEXTURE_PLAIN,
	TEXTURE_CUBEMAP,
	TEXTURE_FONT
};


class VulkanDevice
{
	public:

	#ifdef NDEBUG
		const bool enableValidationLayers = false;
	#else
		const bool enableValidationLayers = true;
	#endif

	VulkanDevice(VulkanWindow &window);
	~VulkanDevice();

	VulkanDevice() = delete;
	VulkanDevice(const VulkanDevice&) = delete;
	VulkanDevice& operator=(const VulkanDevice&) = delete;
	VulkanDevice(VulkanDevice&&) = delete;
	VulkanDevice& operator=(VulkanDevice&&) = delete;

	VkCommandPool	getCommandPool() { return commandPool; }
	VkDevice		device() { return device_; }
	VkSurfaceKHR	surface() { return surface_; }
	VkQueue			graphicsQueue() { return graphicsQueue_; }
	VkQueue			presentQueue() { return presentQueue_; }
	
	SwapChainSupportDetails	getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
	QueueFamilyIndices		findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
	ui32				getMaxSizePushConstants() { return properties.limits.maxPushConstantsSize; };
	ui32				getMaxSizeUniformBuffer() { return properties.limits.maxUniformBufferRange; };
	ui32				getMaxSizeSsbo() { return properties.limits.maxStorageBufferRange; }

	ui32	findMemoryType(ui32 typeFilter, VkMemoryPropertyFlags properties);
	VkFormat	findSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features
	);

	void	createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory
	);
	VkCommandBuffer	beginSingleTimeCommands();
	void	endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void	copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void	copyBufferToImage(
		VkBuffer buffer,
		VkImage image,
		ui32 width,
		ui32 height,
		ui32 layerCount,
		TextureType textureType
	);

	void	createImageWithInfo(
		const VkImageCreateInfo& imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory
	);

	void	transitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		ui32 layerCount
	);

	VkImageView	createImageView(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		ui32 layerCount,
		TextureType textureType = TEXTURE_PLAIN
	);

	VkPhysicalDeviceProperties	properties;

	private:
	
	void	createInstance();
	void	setupDebugMessenger();
	void	createSurface();
	void	pickPhysicalDevice();
	void	createLogicalDevice();
	void	createCommandPool();

	bool	isDeviceSuitable(VkPhysicalDevice device);
	bool	checkValidationLayerSupport();
	bool	checkDeviceExtensionSupport(VkPhysicalDevice device);
	void	hasGflwRequiredInstanceExtensions();
	void	populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

	std::vector<const char*>	getRequiredExtensions();
	QueueFamilyIndices			findQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails		querySwapChainSupport(VkPhysicalDevice device);

	VkInstance					instance;
	VkDebugUtilsMessengerEXT	debugMessenger;
	VkPhysicalDevice			physicalDevice = VK_NULL_HANDLE;
	VulkanWindow&				window;
	VkCommandPool				commandPool;

	VkDevice		device_;
	VkSurfaceKHR	surface_;
	VkQueue			graphicsQueue_;
	VkQueue			presentQueue_;

	const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

}
