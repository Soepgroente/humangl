#include "Vectors.hpp"
#include "VulkanDevice.hpp"
#include "VulkanUtils.hpp"

// std headers
#include <cstring>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <unordered_set>

namespace ve {

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	(void)messageSeverity;
	(void)messageType;
	(void)pUserData;
	return VK_FALSE;
}

VkResult	CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void	DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

VulkanDevice::VulkanDevice(VulkanWindow& window) : window{window}
{
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createCommandPool();
}

VulkanDevice::~VulkanDevice()
{
	vkDestroyCommandPool(device_, commandPool, nullptr);
	vkDestroyDevice(device_, nullptr);

	if (enableValidationLayers == true)
	{
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(instance, surface_, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void VulkanDevice::createInstance()
{
	if (enableValidationLayers == true && checkValidationLayerSupport() == false)
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}
	VkApplicationInfo appInfo 
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "App",
		.applicationVersion = VK_VERSION_1_3,
		.pEngineName = "Engine",
		.engineVersion = VK_API_VERSION_1_3,
		.apiVersion = VK_API_VERSION_1_3
	};

	VkInstanceCreateInfo createInfo
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo
	};
	
	std::vector<const char*> extensions = getRequiredExtensions();

	#ifdef __APPLE__
	extensions.push_back("VK_KHR_portability_enumeration");
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	#endif
	createInfo.enabledExtensionCount = static_cast<ui32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers == true)
	{
		createInfo.enabledLayerCount = static_cast<ui32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	errorCheck(vkCreateInstance(&createInfo, nullptr, &instance), "failed to create instance!");
	hasGflwRequiredInstanceExtensions();
}

void VulkanDevice::pickPhysicalDevice()
{
	ui32 deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	std::multimap<int, VkPhysicalDevice> candidates;

	for (const VkPhysicalDevice& device : devices)
	{
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceProperties2 deviceProperties {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

		vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties);
		if (deviceProperties.properties.apiVersion >= VK_API_VERSION_1_3 && isDeviceSuitable(device) == true)
		{
			int score = 0;
			if (deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				score += 1000;
			}
			score += deviceProperties.properties.limits.maxImageDimension2D;
			candidates.insert(std::make_pair(score, physicalDevice));
		}
	}
	if (candidates.empty() == false && candidates.rbegin()->first > 0)
	{
		physicalDevice = candidates.rbegin()->second;
	}
	else
	{
		throw std::runtime_error("failed to find a suitable render device!");
	}
}

void VulkanDevice::createLogicalDevice()
{
	#ifdef __APPLE__
		deviceExtensions.push_back("VK_KHR_portability_subset");
	#endif

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<ui32> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	f32 queuePriority = 0.5f;
	for (ui32 queueFamily : uniqueQueueFamilies)
	{
		queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		});
	}

	// Core "old-style" features (VkPhysicalDeviceFeatures)
	VkPhysicalDeviceFeatures coreFeatures
	{
		.samplerAnisotropy = VK_TRUE,
		.imageCubeArray = VK_TRUE
	};

	// Vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features enabledVk12Features
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.descriptorIndexing = true,
		.shaderSampledImageArrayNonUniformIndexing = true,
		.descriptorBindingVariableDescriptorCount = true,
		.runtimeDescriptorArray = true,
		.bufferDeviceAddress = true
	};

	// Vulkan 1.3 features (include vk1.2 features)
	VkPhysicalDeviceVulkan13Features features13
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &enabledVk12Features,
		.synchronization2 = true,
		.dynamicRendering = true
	};

	// Descriptor indexing (bindless/non-uniform indexing related)
	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE
	};
	features13.pNext = &indexingFeatures;

	// Query support first via features2 chain
	VkPhysicalDeviceFeatures2 supportedFeatures2
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features13
	};

	vkGetPhysicalDeviceFeatures2(physicalDevice, &supportedFeatures2);

	// Validate required features are actually supported
	if (supportedFeatures2.features.samplerAnisotropy == false ||
		supportedFeatures2.features.imageCubeArray == false ||
		indexingFeatures.shaderSampledImageArrayNonUniformIndexing == false ||
		indexingFeatures.runtimeDescriptorArray == false)
	{
		throw std::runtime_error("Required Vulkan device features are not supported.");
	}

	VkDeviceCreateInfo createInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = static_cast<ui32>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.pNext = &supportedFeatures2,
		.pEnabledFeatures = nullptr,
		.enabledExtensionCount = static_cast<ui32>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data()
	};

	if (enableValidationLayers == true)
	{
		createInfo.enabledLayerCount = static_cast<ui32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}
	errorCheck(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_), "failed to create logical device!");

	vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
	vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);
}

void	VulkanDevice::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

	VkCommandPoolCreateInfo poolInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = queueFamilyIndices.graphicsFamily,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	};

	errorCheck(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool), "failed to create command pool!");
}

void	VulkanDevice::createSurface()
{
	window.createWindowSurface(instance, &surface_);
}

bool	VulkanDevice::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);
	if (indices.isComplete() == false || checkDeviceExtensionSupport(device) == false)
	{
		return false;
	}

	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
	if (swapChainSupport.formats.empty() == true || swapChainSupport.presentModes.empty() == true)
	{
		return false;
	}

	VkPhysicalDeviceProperties props{};
	vkGetPhysicalDeviceProperties(device, &props);
	if (props.apiVersion < VK_API_VERSION_1_3)
	{
		return false;
	}

	VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
	VkPhysicalDeviceDescriptorIndexingFeatures indexing
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
		.pNext = &indexing
	};
	VkPhysicalDeviceFeatures2 features2
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features13
	};

	vkGetPhysicalDeviceFeatures2(device, &features2);

	return	features2.features.samplerAnisotropy == true &&
			features2.features.geometryShader == true &&
			indexing.shaderSampledImageArrayNonUniformIndexing == true &&
			indexing.runtimeDescriptorArray == true &&
			indexing.descriptorBindingPartiallyBound == true &&
			indexing.descriptorBindingVariableDescriptorCount == true;
}

void	VulkanDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void	VulkanDevice::setupDebugMessenger()
{
	if (enableValidationLayers == false)
	{
		return;
	}
	VkDebugUtilsMessengerCreateInfoEXT createInfo;

	populateDebugMessengerCreateInfo(createInfo);
	errorCheck(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger), "failed to set up debug messenger!");
}

bool	VulkanDevice::checkValidationLayerSupport()
{
	ui32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto &layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (layerFound == false)
		{
			return false;
		}
	}
	return true;
}

std::vector<const char*>	VulkanDevice::getRequiredExtensions()
{
	ui32 glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	if (glfwExtensions == nullptr)
	{
		throw std::runtime_error("failed to get required GLFW extensions");
	}
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers == true)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

void	VulkanDevice::hasGflwRequiredInstanceExtensions()
{
	ui32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::unordered_set<std::string> available;
	for (const VkExtensionProperties& extension : extensions)
	{
		available.insert(extension.extensionName);
	}
	std::vector<const char *> requiredExtensions = getRequiredExtensions();
	for (const char* required : requiredExtensions)
	{
		if (available.find(required) == available.end())
		{
			throw std::runtime_error("Missing required glfw extension");
		}
	}
}

bool	VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	ui32 extensionCount;
	if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
	{
		return false;
	}

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(
		device,
		nullptr,
		&extensionCount,
		availableExtensions.data()
	);

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const VkExtensionProperties& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}

QueueFamilyIndices	VulkanDevice::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	ui32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (size_t i = 0; i < queueFamilies.size(); i++)
	{
		if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
			indices.graphicsFamilyHasValue = true;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
		if (queueFamilies[i].queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
			indices.presentFamilyHasValue = true;
		}
		if (indices.isComplete() == true)
		{
			break;
		}
	}
	return indices;
}

SwapChainSupportDetails	VulkanDevice::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

	ui32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
	}

	ui32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device,
			surface_,
			&presentModeCount,
			details.presentModes.data());
	}
	return details;
}

VkFormat	VulkanDevice::findSupportedFormat(
	const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) ||
			(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features))
		{
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

ui32	VulkanDevice::findMemoryType(ui32 typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (ui32 i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (((typeFilter & (1 << i)) != 0) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

void	VulkanDevice::createBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	errorCheck(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer), "failed to create vertex buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
	};

	errorCheck(vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory), "failed to allocate vertex buffer memory!");

	vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}

VkCommandBuffer	VulkanDevice::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = commandPool,
		.commandBufferCount = 1
	};

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void	VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};

	vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue_);

	vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
}

void	VulkanDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	VkBufferCopy copyRegion{.size = size};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	endSingleTimeCommands(commandBuffer);
}

void	VulkanDevice::copyBufferToImage(
	VkBuffer buffer, VkImage image, ui32 width, ui32 height, ui32 layerCount, TextureType textureType)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	if (textureType == TEXTURE_PLAIN || textureType == TEXTURE_FONT)
	{
		VkBufferImageCopy region
		{
			region.bufferOffset = 0,
			region.bufferRowLength = 0,
			region.bufferImageHeight = 0,
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			region.imageSubresource.mipLevel = 0,
			region.imageSubresource.baseArrayLayer = 0,
			region.imageSubresource.layerCount = layerCount,
			region.imageOffset = {0, 0, 0},
			region.imageExtent = {width, height, 1}
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
	}
	else if (textureType == TEXTURE_CUBEMAP)
	{
		ui32 faceWidth = width / 4;
		ui32 faceHeight = height / 3;
		ui32 faceSize = faceWidth * faceHeight * 4;

		std::vector<VkBufferImageCopy> regions(6);
		for (ui32 face = 0; face < 6; face++)
		{
			regions[face].bufferOffset = face * faceSize;
			regions[face].bufferRowLength = 0;
			regions[face].bufferImageHeight = 0;
			regions[face].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			regions[face].imageSubresource.mipLevel = 0;
			regions[face].imageSubresource.baseArrayLayer = face;
			regions[face].imageSubresource.layerCount = 1;
			regions[face].imageOffset = {0, 0, 0};
			regions[face].imageExtent = {faceWidth, faceHeight, 1};
		}

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<ui32>(regions.size()),
			regions.data()
		);
	}
	endSingleTimeCommands(commandBuffer);
}

void	VulkanDevice::createImageWithInfo(
	const VkImageCreateInfo& imageInfo,
	VkMemoryPropertyFlags properties,
	VkImage& image,
	VkDeviceMemory& imageMemory)
{
	errorCheck(vkCreateImage(device_, &imageInfo, nullptr, &image), "failed to create image!");
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device_, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
	};

	errorCheck(vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory), "failed to allocate image memory!");
	errorCheck(vkBindImageMemory(device_, image, imageMemory, 0), "failed to bind image memory!");
}

void	VulkanDevice::transitionImageLayout(
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	ui32 layerCount
	)
{
	(void)format;
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	VkImageMemoryBarrier barrier
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.levelCount = 1,
		.subresourceRange.layerCount = layerCount
	};

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

VkImageView	VulkanDevice::createImageView(
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	ui32 layerCount,
	TextureType textureType)
{
	VkComponentMapping components
	{
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY
	};
	VkImageViewCreateInfo viewInfo
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.image = image,
		.format = format,
		.components =
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange.aspectMask = aspectFlags,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = layerCount,
	};
	if (textureType == TEXTURE_CUBEMAP)
	{
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}

	VkImageView imageView{};

	errorCheck(vkCreateImageView(device_, &viewInfo, nullptr, &imageView), "failed to create texture image view!");
	return imageView;
}

}	// namespace ve