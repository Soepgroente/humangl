#include "VulkanDescriptors.hpp"
#include "VulkanTexture.hpp"

#include <cassert>


namespace ve {

BindInfo::~BindInfo() {}

ui32 VulkanBindingSet::ID_INSTANCE = 0U;

VulkanBindingSet&	VulkanBindingSet::addBufferBinding(ui32 binding, VkShaderStageFlags stage, ui32 bufferSize, BufferType bufferType)
{
	if (bufferType != BUFFER_UNIFORM and bufferType != BUFFER_STORAGE)
	{
		throw std::runtime_error("buffer type can be only uniform or storage");
	}

	VkDescriptorSetLayoutBinding newBinding{};

	newBinding.binding = binding;
	newBinding.descriptorType = (bufferType == BUFFER_UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	newBinding.descriptorCount = 1;
	newBinding.stageFlags = stage;

	this->bindings.emplace_back(std::make_unique<UniformBindInfo>(newBinding, bufferSize, bufferType));
	return *this;
}

VulkanBindingSet&	VulkanBindingSet::addSamplerBinding(ui32 binding, VkShaderStageFlags stage, const std::string& texturePath, TextureType textureType)
{
	VkDescriptorSetLayoutBinding newBinding{};

	newBinding.binding = binding;
	newBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	newBinding.descriptorCount = 1;
	newBinding.stageFlags = stage;

	this->bindings.emplace_back(std::make_unique<SamplerBindInfo>(newBinding, texturePath, textureType));
	return *this;
}

VulkanBindingSet&	VulkanBindingSet::addBufferArrayBinding(ui32 binding, VkShaderStageFlags stage, const std::vector<ui32>& sizes, BufferType bufferType)
{
	if (bufferType != BUFFER_UNIFORM and bufferType != BUFFER_STORAGE)
	{
		throw std::runtime_error("buffer type can be only uniform or storage");
	}

	ui32 nBuffers = sizes.size();
	assert(nBuffers > 0U and "no buffer size provided");

	VkDescriptorSetLayoutBinding newBinding{};
	newBinding.binding = binding;
	newBinding.descriptorType = (bufferType == BUFFER_UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	newBinding.descriptorCount = nBuffers;
	newBinding.stageFlags = stage;

	this->bindings.emplace_back(std::make_unique<UniformBindInfo>(newBinding, sizes, bufferType));
	return *this;
}

VulkanBindingSet&	VulkanBindingSet::addSamplerArrayBinding(ui32 binding, VkShaderStageFlags stage, const std::vector<std::string>& texturePaths, std::vector<TextureType> types)
{
	if (texturePaths.size() != types.size())
	{
		throw std::runtime_error("texture paths and texture types differ in quantity");
	}

	ui32 nSamplers = texturePaths.size();
	assert(nSamplers > 0U and "no texture info provided");

	VkDescriptorSetLayoutBinding newBinding{};

	newBinding.binding = binding;
	newBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	newBinding.descriptorCount = nSamplers;
	newBinding.stageFlags = stage;

	this->bindings.emplace_back(std::make_unique<SamplerBindInfo>(newBinding, texturePaths, types));
	return *this;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindingSet::getVkBindingData() const noexcept
{
	std::vector<VkDescriptorSetLayoutBinding> vkBindings(this->bindings.size());

	for (ui32 i = 0U; i < this->bindings.size(); i++)
	{
		vkBindings[i] = this->bindings[i]->vkInfo;
	}
	return vkBindings;
}


VulkanDescriptorSetFactory::~VulkanDescriptorSetFactory() noexcept
{
	for (VkDescriptorSetLayout layoutSet : this->descriptorSetlayouts)
	{
		vkDestroyDescriptorSetLayout(this->vulkanDevice.device(), layoutSet, nullptr);
	}
	this->existingLayouts.clear();
	this->descriptorSetlayouts.clear();

	if (this->descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(this->vulkanDevice.device(), this->descriptorPool, nullptr);
	}
}

VulkanDescriptorSetFactory::VulkanDescriptorSetFactory(VulkanDescriptorSetFactory& other) noexcept :
	vulkanDevice{other.vulkanDevice}
{
	if (this != &other)
	{
		this->poolFlags = other.poolFlags;
		this->maxSets = other.maxSets;
		this->countTypes = std::move(other.countTypes);
		this->descriptorPool = other.descriptorPool;
		this->existingLayouts = std::move(other.existingLayouts);
		this->descriptorSetlayouts = std::move(other.descriptorSetlayouts);

		other.descriptorPool = VK_NULL_HANDLE;
	}
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addPoolSize(VkDescriptorType type, ui32 count)
{
	if (this->countTypes.count(type) == 0)
	{
		throw std::runtime_error("only descriptor type supported: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER");
	}

	this->countTypes[type] += count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addBufferPoolSize(ui32 count)
{
	this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] += count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addSsboPoolSize(ui32 count)
{
	this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] += count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::addSamplerPoolSize(ui32 count)
{
	this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] += count;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::setPoolFlags(VkDescriptorPoolCreateFlags flags) noexcept
{
	this->poolFlags |= flags;
	return *this;
}

VulkanDescriptorSetFactory&	VulkanDescriptorSetFactory::setMaxSets(ui32 count) noexcept
{
	this->maxSets = count;
	return *this;
}

VulkanDescriptorSetFactory& VulkanDescriptorSetFactory::createPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes{};
	if (this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] > 0)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]
		});
	}
	if (this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] > 0)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER]
		});
	}
	if (this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] > 0)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER]
		});
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = this->maxSets;
	poolInfo.flags = this->poolFlags;

	if (vkCreateDescriptorPool(this->vulkanDevice.device(), &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
	return *this;
}

VulkanDescriptorSetFactory& VulkanDescriptorSetFactory::resetPool() noexcept
{
	for (VkDescriptorSetLayout layoutSet : this->descriptorSetlayouts)
	{
		vkDestroyDescriptorSetLayout(this->vulkanDevice.device(), layoutSet, nullptr);
	}
	this->existingLayouts.clear();
	this->descriptorSetlayouts.clear();

	if (this->descriptorPool != VK_NULL_HANDLE)
	{
		// automatically also clears every descriptor set created by this pool
		vkResetDescriptorPool(this->vulkanDevice.device(), this->descriptorPool, 0);
		this->descriptorPool = VK_NULL_HANDLE;
	}

	this->poolFlags = 0U;
	this->maxSets = 0U;
	this->countTypes.clear();
	return *this;
}

std::unique_ptr<VulkanDescriptorSet>	VulkanDescriptorSetFactory::createDescriptorSet(const VulkanBindingSet& bindings)
{
	if (this->descriptorPool == VK_NULL_HANDLE)
	{
		throw std::runtime_error("pool not created");
	}
	else if(this->maxSets == 0U)
	{
		throw std::runtime_error("not enough sets left from the pool");
	}
	this->maxSets--;

	ui32 countUBOs = 0U, countSSBOs = 0U, countSamplers = 0U;
	for (const std::unique_ptr<BindInfo>& bindData : bindings.getBindingData())
	{
		if (dynamic_cast<UniformBindInfo*>(bindData.get()))
		{
			UniformBindInfo* uniformBinding	= dynamic_cast<UniformBindInfo*>(bindData.get());
			(uniformBinding->getBufferType() == BUFFER_UNIFORM) ? countUBOs += uniformBinding->getNitems() : countSSBOs += uniformBinding->getNitems();

			for (ui32 size : uniformBinding->getBufferSizes())
			{
				if (size > this->vulkanDevice.getMaxSizeUniformBuffer())
				{
					throw std::runtime_error("uniform buffer size exceeds device limit");
				}
			}
		}
		else if (dynamic_cast<SamplerBindInfo*>(bindData.get()))
		{
			countSamplers += bindData->getNitems();
		}
	}

	if ((countUBOs + countSSBOs + countSamplers) == 0U)
	{
		throw std::runtime_error("no binding set for descriptor set creation");
	}

	if(this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] < countUBOs)
	{
		throw std::runtime_error("not enough uniform buffer descriptors, create a new pool");
	}
	this->countTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] -= countUBOs;

	if(this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] < countSSBOs)
	{
		throw std::runtime_error("not enough storage buffer descriptors, create a new pool");
	}
	this->countTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] -= countSSBOs;

	if(this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] < countSamplers)
	{
		throw std::runtime_error("not enough sampler descriptors, create a new pool");
	}
	this->countTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] -= countSamplers;

	// create a new descLayout if none is found linked to this binding
	if (this->existingLayouts.count(bindings.getId()) == 0U)
	{
		this->addNewLayout(bindings);
	}

	return std::make_unique<VulkanDescriptorSet>(
		this->vulkanDevice,
		this->existingLayouts[bindings.getId()],
		this->descriptorPool,
		bindings
	);
}

void	VulkanDescriptorSetFactory::addNewLayout(const VulkanBindingSet& bindings)
{
	std::vector<VkDescriptorSetLayoutBinding>	vkBindings = bindings.getVkBindingData();
	VkDescriptorSetLayout						newLayout;

	VkDescriptorSetLayoutCreateInfo	descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = vkBindings.size();
	descriptorSetLayoutInfo.pBindings = vkBindings.data();

	if (vkCreateDescriptorSetLayout(
			this->vulkanDevice.device(),
			&descriptorSetLayoutInfo,
			nullptr,
			&newLayout
		) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout");
	}
	this->descriptorSetlayouts.push_back(newLayout);
	this->existingLayouts[bindings.getId()] = this->descriptorSetlayouts.back();
}


VulkanDescriptorSet::VulkanDescriptorSet(
	VulkanDevice&			vulkanDevice,
	VkDescriptorSetLayout	descriptorSetLayout,
	VkDescriptorPool		descriptorPool,
	const VulkanBindingSet&	bindings
) : descriptorSetLayout{descriptorSetLayout}
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &this->descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	if (vkAllocateDescriptorSets(vulkanDevice.device(), &allocInfo, &this->descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set");
	}

	for(const std::unique_ptr<BindInfo>& bindData : bindings.getBindingData())
	{
		if (dynamic_cast<UniformBindInfo*>(bindData.get()) != nullptr)
		{
			UniformBindInfo* uniformBinding	= dynamic_cast<UniformBindInfo*>(bindData.get());
			this->descriptors[bindData->getBinding()] = std::make_unique<VulkanBufferDescriptor>(*uniformBinding, vulkanDevice, this->descriptorSet);
		}
		else if (dynamic_cast<SamplerBindInfo*>(bindData.get()) != nullptr)
		{
			SamplerBindInfo* samplerBinding	= dynamic_cast<SamplerBindInfo*>(bindData.get());
			this->descriptors[bindData->getBinding()] = std::make_unique<VulkanSamplerDescriptor>(*samplerBinding, vulkanDevice, this->descriptorSet);
		}
	}
}

void	VulkanDescriptorSet::updateDescriptor(ui32 binding, const void* data, ui32 index) noexcept
{
	assert(this->descriptors.count(binding) != 0U && "Binding not found in descriptor set");

	this->descriptors[binding]->update(data, index);
}

void VulkanDescriptorSet::bindSet(VkCommandBuffer commandBuffer, VulkanPipeline const& pipeline, ui32 setIndex) noexcept
{
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline.getPipelineLayout(),
		setIndex,
		1,
		&this->descriptorSet,
		0,
		nullptr
	);
}

const VulkanBufferDescriptor* VulkanDescriptorSet::getBufferDescriptor(ui32 binding) const noexcept
{
	assert(this->descriptors.count(binding) != 0U && "Binding not found in descriptor set");
	assert(dynamic_cast<VulkanBufferDescriptor*>(this->descriptors.at(binding).get()) && "Binding is not a buffer descriptor");

	return dynamic_cast<VulkanBufferDescriptor*>(this->descriptors.at(binding).get());
}

const VulkanSamplerDescriptor* VulkanDescriptorSet::getSamplerDescriptor(ui32 binding) const noexcept
{
	assert(this->descriptors.count(binding) != 0U && "Binding not found in descriptor set");
	assert(dynamic_cast<VulkanSamplerDescriptor*>(this->descriptors.at(binding).get()) && "Binding is not a sampler descriptor");

	return dynamic_cast<VulkanSamplerDescriptor*>(this->descriptors.at(binding).get());
}


VulkanBufferDescriptor::VulkanBufferDescriptor(const UniformBindInfo& binding, VulkanDevice& vulkanDevice, VkDescriptorSet descriptorSet)
	: VulkanDescriptor(binding)
{
	const std::vector<ui32>&	bufferSizes = binding.getBufferSizes();
	BufferType					bufferType = binding.getBufferType();
	ui32 						nBuffers = binding.getNitems();

	std::vector<VkDescriptorBufferInfo> bufferInfo(nBuffers);
	this->buffers.resize(nBuffers);

	for(ui32 i = 0U; i < nBuffers; i++)
	{
		this->buffers[i] = std::make_unique<VulkanBuffer>(
			vulkanDevice,
			bufferSizes[i],
			1,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			bufferType
		);
		this->buffers[i]->map();

		bufferInfo[i] = this->buffers[i]->descriptorBufferInfo();
	}

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = this->binding;
	write.descriptorType = (bufferType == BUFFER_UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.descriptorCount = nBuffers;
	write.dstSet = descriptorSet;
	write.pBufferInfo = bufferInfo.data();

	vkUpdateDescriptorSets(vulkanDevice.device(), 1, &write, 0, nullptr);
}

void	VulkanBufferDescriptor::update(const void* data, ui32 index) noexcept
{
	assert(index < this->buffers.size() && "Buffer binding not found in descriptor set");

	this->buffers[index]->writeToBuffer(data);
}

VulkanSamplerDescriptor::VulkanSamplerDescriptor(const SamplerBindInfo& binding, VulkanDevice& vulkanDevice, VkDescriptorSet descriptorSet)
	: VulkanDescriptor(binding)
{
	const std::vector<std::string>&	texturePaths = binding.getTexturePaths();
	std::vector<TextureType>		textureTypes = binding.getTextureTypes();
	ui32 							nTextures = binding.getNitems();

	std::vector<VkDescriptorImageInfo> textureInfo(nTextures);
	this->textures.resize(nTextures);

	for (ui32 i = 0; i < nTextures; i++)
	{
		this->textures[i] = std::make_unique<VulkanTexture>(vulkanDevice, texturePaths[i], textureTypes[i]);
		textureInfo[i] = this->textures[i]->getDescriptorImageInfo();
	}

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = this->binding;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = nTextures;
	write.dstSet = descriptorSet;
	write.pImageInfo = textureInfo.data();

	vkUpdateDescriptorSets(vulkanDevice.device(), 1, &write, 0, nullptr);
}

void	VulkanSamplerDescriptor::update(const void* data, ui32 index) noexcept
{
	(void) data;
	(void) index;
}

FontModel	VulkanSamplerDescriptor::getModelFromText(const std::string& text, const vec2i& origin, ui32 index, bool isRightAligned) const noexcept
{
	assert(index < this->textures.size() && "Texture index not found in descriptor");

	return this->textures[index]->getModelFromText(text, origin, isRightAligned);	
}

}	// namespace ve
