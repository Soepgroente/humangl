#include "VulkanSwapChain.hpp"
#include "VulkanUtils.hpp"

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace ve {

VulkanSwapChain::VulkanSwapChain(VulkanDevice& deviceRef, VkExtent2D extent) : device{deviceRef}, extent{extent}
{
	init();
}

VulkanSwapChain::VulkanSwapChain(VulkanDevice& deviceRef, VkExtent2D extent, std::shared_ptr<VulkanSwapChain> previous) 
	: device{deviceRef}, extent{extent}, oldSwapChain{previous}
{
	init();

	oldSwapChain = nullptr;
}

void	VulkanSwapChain::init()
{
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDepthResources();
	createFramebuffers();
	createSyncVulkanObjects();
}

VulkanSwapChain::~VulkanSwapChain()
{
	for (VkImageView imageView : swapChainImageViews)
	{
		vkDestroyImageView(device.device(), imageView, nullptr);
	}
	swapChainImageViews.clear();

	if (swapChain != nullptr)
	{
		vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
		swapChain = nullptr;
	}

	for (size_t i = 0; i < depthImages.size(); i++)
	{
		vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
		vkDestroyImage(device.device(), depthImages[i], nullptr);
		vkFreeMemory(device.device(), depthImageMemorys[i], nullptr);
	}

	for (VkFramebuffer framebuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(device.device(), renderPass, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device.device(), inFlightFences[i], nullptr);
	}
	for (size_t i = 0; i < imageCount(); i++)
	{
		vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
	}
}

VkResult	VulkanSwapChain::acquireNextImage(ui32* imageIndex) noexcept
{
	vkWaitForFences(
		device.device(),
		1,
		&inFlightFences[currentFrame],
		VK_TRUE,
		std::numeric_limits<uint64_t>::max()
	);

	VkResult result = vkAcquireNextImageKHR(
		device.device(),
		swapChain,
		std::numeric_limits<uint64_t>::max(),
		imageAvailableSemaphores[currentFrame],
		VK_NULL_HANDLE,
		imageIndex
	);

	return result;
}

VkResult	VulkanSwapChain::submitCommandBuffers(const VkCommandBuffer* buffers, ui32* imageIndex)
{
	if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
	}
	imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

	VkSubmitInfo submitInfo = {};

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = buffers;

	VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[*imageIndex]};

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
	errorCheck(vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]), "failed to submit draw command buffer!");

	VkPresentInfoKHR presentInfo = {};

	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {swapChain};

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = imageIndex;

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	VkResult result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

	return result;
}

static ui32	chooseImageCount(const ve::SwapChainSupportDetails& scsd)
{
	ui32 imageCount = scsd.capabilities.minImageCount + 1;

	if (scsd.capabilities.maxImageCount > 0 && imageCount > scsd.capabilities.maxImageCount)
	{
		imageCount = scsd.capabilities.maxImageCount;
	}
	return imageCount;
}

void	VulkanSwapChain::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();
	this->surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	this->extent = chooseSwapExtent(swapChainSupport.capabilities);
	ui32 imageCount = chooseImageCount(swapChainSupport);

	VkSwapchainCreateInfoKHR createInfo
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = device.surface(),
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = swapChainSupport.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = chooseSwapPresentMode(swapChainSupport.presentModes),
		.clipped = VK_TRUE,
		.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain
	};

	QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
	ui32 queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	errorCheck(vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain), "failed to create swap chain!");
	vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
}

void	VulkanSwapChain::createImageViews()
{
	size_t size = swapChainImages.size();

	swapChainImageViews.resize(size);
	for (size_t i = 0; i < size; i++)
	{
		swapChainImageViews[i] = device.createImageView(swapChainImages[i],
														swapChainImageFormat,
														VK_IMAGE_ASPECT_COLOR_BIT,
														1);
	}
}

void	VulkanSwapChain::createRenderPass()
{
	VkAttachmentDescription depthAttachment{};

	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};

	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachment = {};

	colorAttachment.format = getSwapChainImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};

	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};

	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};

	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcAccessMask = 0;
	dependency.srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstSubpass = 0;
	dependency.dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo = {};

	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<ui32>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	errorCheck(vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass), "failed to create render pass!");
}

void	VulkanSwapChain::createFramebuffers()
{
	size_t count = imageCount();

	swapChainFramebuffers.resize(count);
	for (size_t i = 0; i < count; i++)
	{
		std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageViews[i]};
		VkFramebufferCreateInfo framebufferInfo = {};

		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<ui32>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		errorCheck(vkCreateFramebuffer(
				device.device(),
				&framebufferInfo,
				nullptr,
				&swapChainFramebuffers[i]), "failed to create framebuffer!");
	}
}

void	VulkanSwapChain::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat();
	swapChainDepthFormat = depthFormat;

	depthImages.resize(imageCount());
	depthImageMemorys.resize(imageCount());
	depthImageViews.resize(imageCount());

	for (size_t i = 0; i < depthImages.size(); i++)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = swapChainExtent.width;
		imageInfo.extent.height = swapChainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = depthFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = 0;

		device.createImageWithInfo(
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImages[i],
			depthImageMemorys[i]);

		VkImageViewCreateInfo viewInfo{};

		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = depthImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		errorCheck(vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageViews[i]), "failed to create texture image view!");
	}
}

void	VulkanSwapChain::createSyncVulkanObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(imageCount());
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		errorCheck(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]), "failed to create synchronization VulkanObjects for a frame!");
		errorCheck(vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]), "failed to create synchronization VulkanObjects for a frame!");
	}
	for (size_t i = 0; i < imageCount(); i++)
	{
		errorCheck(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]),
			"failed to create synchronization VulkanObjects for a frame!");
	}
}

VkSurfaceFormatKHR	VulkanSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const noexcept
{
	assert(availableFormats.empty() == false && "No available formats for window surface");
	for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR	VulkanSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const noexcept
{
	for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			std::cout << "Present mode: Mailbox available" << std::endl;
			return availablePresentMode;
		}
	}
	std::cout << "Choosing present mode: V-Sync" << std::endl;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D	VulkanSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)	const noexcept
{
	if (capabilities.currentExtent.width != std::numeric_limits<ui32>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = extent;

		actualExtent.width = std::max(
			capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(
			capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VkFormat	VulkanSwapChain::findDepthFormat()
{
	return device.findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool	VulkanSwapChain::compareSwapFormats(const VulkanSwapChain& otherSwapChain) const noexcept
{
	return swapChainImageFormat == otherSwapChain.swapChainImageFormat &&
		swapChainDepthFormat == otherSwapChain.swapChainDepthFormat;
}

} // namespace ve