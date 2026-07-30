#pragma once

#include "VulkanDevice.hpp"
#include "../type_aliases.hpp"

namespace ve {

enum BufferType
{
	BUFFER_UNIFORM,
	BUFFER_STORAGE,
	BUFFER_VERTEX,
	BUFFER_INDEX,
	BUFFER_RAW
};

class VulkanBuffer
{
	public:

	VulkanBuffer(
		VulkanDevice& device,
		VkDeviceSize instanceSize,
		ui32 instanceCount,
		VkMemoryPropertyFlags memoryPropertyFlags,
		BufferType bufferType,
		VkDeviceSize minOffsetAlignment = 1);
	~VulkanBuffer();

	VulkanBuffer() = delete;
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer(VulkanBuffer&&);
	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	VkResult	map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) noexcept;
	void		unmap() noexcept;

	void		writeToBuffer(const void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) noexcept;
	VkResult	flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) noexcept;
	
	VkDescriptorBufferInfo	descriptorBufferInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const noexcept;
	VkResult				invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) noexcept;

	void		writeToIndex(const void* data, i32 index) noexcept;
	VkResult	flushIndex(i32 index) noexcept;
	VkDescriptorBufferInfo	descriptorInfoForIndex(i32 index) noexcept;
	VkResult	invalidateIndex(i32 index) noexcept;

	VkBuffer		getBuffer() const noexcept { return buffer; }
	void*			getMappedMemory() const noexcept { return mapped; }
	ui32			getInstanceCount() const noexcept { return instanceCount; }
	VkDeviceSize	getInstanceSize() const noexcept { return instanceSize; }
	VkDeviceSize	getAlignmentSize() const noexcept { return instanceSize; }
	VkDeviceSize	getBufferSize() const noexcept { return bufferSize; }

	VkBufferUsageFlags		getUsageFlags() const noexcept { return usageFlags; }
	VkMemoryPropertyFlags	getMemoryPropertyFlags() const noexcept { return memoryPropertyFlags; }

	private:

	static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) noexcept;

	VulkanDevice&	vulkanDevice;
	void*			mapped = nullptr;
	VkBuffer		buffer = VK_NULL_HANDLE;
	VkDeviceMemory	memory = VK_NULL_HANDLE;

	VkDeviceSize	bufferSize;
	VkDeviceSize	instanceSize;
	ui32			instanceCount;
	VkDeviceSize	alignmentSize;

	VkBufferUsageFlags		usageFlags;
	VkMemoryPropertyFlags	memoryPropertyFlags;
};

}	// namespace lve