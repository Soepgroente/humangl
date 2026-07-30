#include "VulkanModel.hpp"
#include "VulkanObject.hpp"

#include <cassert>


namespace ve {

VulkanModel::VulkanModel(
	VulkanDevice& device,
	const Builder& builder,
	ui32 binding,
	MeshLayout layout
) :
	vulkanDevice{device}, binding{binding}, layout{layout}
{
	this->createVertexBuffer(builder.vertices);
	if (builder.indices.size() > 2U)
	{
		this->createIndexBuffer(builder.indices);
	}
}

VulkanModel::VulkanModel(
	VulkanDevice& device,
	const std::vector<Vertex>& vertices,
	const std::vector<ui32>& indices,
	ui32 binding,
	MeshLayout layout
) :
	vulkanDevice{device}, binding{binding}, layout{layout}
{
	this->createVertexBuffer(vertices);
	if (indices.size() > 2U)
	{
		this->createIndexBuffer(indices);
	}
}

void	VulkanModel::bindBuffer(VkCommandBuffer commandBuffer) const noexcept
{
	VkBuffer		buffers[] = {this->vertexBuffer->getBuffer()};
	VkDeviceSize	offsets[] = {0};

	vkCmdBindVertexBuffers(commandBuffer, this->binding, 1, buffers, offsets);
	if (this->isIndexed == true)
	{
		vkCmdBindIndexBuffer(commandBuffer, this->indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void	VulkanModel::draw(VkCommandBuffer commandBuffer) const noexcept
{
	if (this->isIndexed == true)
	{
		vkCmdDrawIndexed(commandBuffer, this->indexCount, 1, 0, 0, 0);
	}
	else
	{
		vkCmdDraw(commandBuffer, this->vertexCount, 1, 0, 0);
	}
}


void	VulkanModel::setBoundingBox(const std::vector<Vertex>& vertices) noexcept
{
	this->boundingBox.min.x = std::min_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.x < b.pos.x; })->pos.x;
	this->boundingBox.max.x = std::max_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.x < b.pos.x; })->pos.x;
	this->boundingBox.min.y = std::min_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.y < b.pos.y; })->pos.y;
	this->boundingBox.max.y = std::max_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.y < b.pos.y; })->pos.y;
	this->boundingBox.min.z = std::min_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.z < b.pos.z; })->pos.z;
	this->boundingBox.max.z = std::max_element(vertices.begin(), vertices.end(),
		[](const Vertex& a, const Vertex& b) { return a.pos.z < b.pos.z; })->pos.z;
}

void	VulkanModel::createVertexBuffer(const std::vector<Vertex>& vertices)
{
	this->vertexCount = static_cast<ui32>(vertices.size());
	assert(this->vertexCount >= 3 && "Vertex count must be at least 3");

	ui32	vertexSize = 0U;
	if (this->layout & MeshLayout::VERTEX) vertexSize += sizeof(vec3);
	if (this->layout & MeshLayout::NORMAL) vertexSize += sizeof(vec3);
	if (this->layout & MeshLayout::TEXTURE) vertexSize += sizeof(vec2);
	if (this->layout & MeshLayout::RANDOM_INDEX_TEXT) vertexSize += sizeof(ui32);
	assert(vertexSize > 0U && "Empty layout for model");

	VulkanBuffer	stagingBuffer(
		this->vulkanDevice,
		vertexSize,
		this->vertexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		BUFFER_RAW
	);
	stagingBuffer.map();

	if (this->layout == DEFAULT_MODEL_LAYOUT)
	{
		stagingBuffer.writeToBuffer(static_cast<const void*>(vertices.data()));
	}
	else
	{
		ui32 offset = 0U;
		for (VulkanModel::Vertex const& vertex : vertices)
		{
			if (this->layout & MeshLayout::VERTEX)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.pos), sizeof(vec3), offset);
				offset += sizeof(vec3);
			}
			if (this->layout & MeshLayout::NORMAL)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.normal), sizeof(vec3), offset);
				offset += sizeof(vec3);
			}
			if (this->layout & MeshLayout::TEXTURE)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.textureUv), sizeof(vec2), offset);
				offset += sizeof(vec2);
			}
			if (this->layout & MeshLayout::RANDOM_INDEX_TEXT)
			{
				stagingBuffer.writeToBuffer(static_cast<const void*>(&vertex.textureIndex), sizeof(ui32), offset);
				offset += sizeof(ui32);
			}
		}
	}
	stagingBuffer.flush();

	this->vertexBuffer = std::make_unique<VulkanBuffer>(
		this->vulkanDevice,
		vertexSize,
		this->vertexCount,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		BUFFER_VERTEX
	);

	VkDeviceSize	bufferSize = vertexSize * this->vertexCount;
	this->vulkanDevice.copyBuffer(stagingBuffer.getBuffer(), this->vertexBuffer->getBuffer(), bufferSize);
}

void	VulkanModel::createIndexBuffer(const std::vector<ui32>& indices)
{
	this->indexCount = static_cast<ui32>(indices.size());
	assert(this->indexCount >= 3 && "Index count must be at least 3");

	ui32		indexSize = sizeof(ui32);
	VkDeviceSize	bufferSize = indexSize * this->indexCount;

	VulkanBuffer	stagingBuffer(
		this->vulkanDevice,
		indexSize,
		this->indexCount,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		BUFFER_RAW
	);

	stagingBuffer.map();
	stagingBuffer.writeToBuffer(static_cast<const void*>(indices.data()));
	stagingBuffer.flush();

	this->indexBuffer = std::make_unique<VulkanBuffer>(
		this->vulkanDevice,
		indexSize,
		this->indexCount,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		BUFFER_INDEX
	);
	this->vulkanDevice.copyBuffer(stagingBuffer.getBuffer(), this->indexBuffer->getBuffer(), bufferSize);
	this->isIndexed = true;
}

void	VulkanModel::setObjectCenter() noexcept
{
	this->boundingCenter = (this->boundingBox.min + this->boundingBox.max) / 2.0f;
}

vec3	VulkanModel::calculateVertexCenter(const std::vector<Vertex>& vertices) noexcept
{
	vec3	center{};

	for (const Vertex& vertex : vertices)
	{
		center += vertex.pos;
	}
	center /= static_cast<f32>(vertices.size());
	return center;
}

MeshLayoutDescription	VulkanModel::getModelLayout(ui32 binding, MeshLayout layout) noexcept
{
	MeshLayoutDescription data{};
	data.bindingConfig.resize(1);

	data.bindingConfig[0].binding = binding;
	data.bindingConfig[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	ui32 locationIndex = 0U, offset = 0U;
	if (layout & MeshLayout::VERTEX)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32B32_SFLOAT, offset}
		);
		offset += sizeof(vec3);
	}
	if (layout & MeshLayout::NORMAL)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32B32_SFLOAT, offset}
		);
		offset += sizeof(vec3);
	}
	if (layout & MeshLayout::TEXTURE)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32G32_SFLOAT, offset}
		);
		offset += sizeof(vec2);
	}
	if (layout & MeshLayout::RANDOM_INDEX_TEXT)
	{
		data.attributeConfig.push_back(
			VkVertexInputAttributeDescription{locationIndex++, binding, VK_FORMAT_R32_UINT, offset}
		);
		offset += sizeof(ui32);
	}
	data.bindingConfig[0].stride = offset;
	return data;
}

void	VulkanModel::Builder::emptyData() noexcept
{
	this->vertices.clear();
	this->indices.clear();
}

}	// namespace ve
