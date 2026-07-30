#pragma once

#include "VulkanDevice.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanBuffer.hpp"

#include <memory>
#include <vector>
#include <map>


namespace ve {

class VulkanBindingSet;

class BindInfo
{
	public:
		BindInfo() = delete;
		BindInfo(const VkDescriptorSetLayoutBinding& vkInfo) noexcept : vkInfo{vkInfo} {};
		virtual ~BindInfo() = 0;
		BindInfo(const BindInfo& other) = default;
		BindInfo(BindInfo&& other) = default;
		BindInfo& operator=(const BindInfo& other) = default;
		BindInfo& operator=(BindInfo& other) = default;
		
		ui32	getBinding() const noexcept { return this->vkInfo.binding; }
		ui32	getNitems() const noexcept { return this->vkInfo.descriptorCount; }

	protected:
		VkDescriptorSetLayoutBinding	vkInfo;

	friend class VulkanBindingSet;
};

class UniformBindInfo : public BindInfo
{
	public:
		UniformBindInfo(const VkDescriptorSetLayoutBinding& vkInfo, ui32 bufferSize, BufferType bufferType)
			:	BindInfo(vkInfo), bufferType{bufferType}
		{
			this->bufferSizes.push_back(bufferSize);
		}

		UniformBindInfo(const VkDescriptorSetLayoutBinding& vkInfo, const std::vector<ui32>& bufferSizes, BufferType bufferType) noexcept
			:	BindInfo(vkInfo), bufferSizes{bufferSizes}, bufferType{bufferType} {}

		const std::vector<ui32>&	getBufferSizes() const noexcept { return bufferSizes; }
		BufferType					getBufferType() const noexcept { return bufferType; }

	private:
		std::vector<ui32>		bufferSizes;
		BufferType				bufferType;
};

class SamplerBindInfo : public BindInfo
{
	public:
		SamplerBindInfo(const VkDescriptorSetLayoutBinding& vkInfo, const std::string& texturePath, TextureType textureType)
			:	BindInfo(vkInfo)
		{
			this->texturePaths.push_back(texturePath);
			this->textureTypes.push_back(textureType);
		}

		SamplerBindInfo(const VkDescriptorSetLayoutBinding& vkInfo, const std::vector<std::string>& texturePaths, const std::vector<TextureType>& textureTypes) noexcept
			:	BindInfo(vkInfo), texturePaths{texturePaths}, textureTypes{textureTypes} {}

		const std::vector<std::string>&	getTexturePaths() const noexcept { return texturePaths; }
		const std::vector<TextureType>&	getTextureTypes() const noexcept { return textureTypes; }

	private:
		std::vector<std::string>	texturePaths;
		std::vector<TextureType>	textureTypes;
};

class VulkanBindingSet
{
	public:
		VulkanBindingSet() noexcept : id{VulkanBindingSet::ID_INSTANCE++} {}
		VulkanBindingSet(const VulkanBindingSet& other) = delete;
		VulkanBindingSet(VulkanBindingSet&& other) = default;
		VulkanBindingSet& operator=(const VulkanBindingSet& other) = delete;
		VulkanBindingSet& operator=(VulkanBindingSet&& other) = delete;

		VulkanBindingSet&	addBufferBinding(ui32 binding, VkShaderStageFlags stage, ui32 bufferSize, BufferType bufferType = BUFFER_UNIFORM);
		VulkanBindingSet&	addSamplerBinding(ui32 binding, VkShaderStageFlags stage, const std::string& texturePath, TextureType textureInfo = TEXTURE_PLAIN);

		VulkanBindingSet&	addBufferArrayBinding(ui32 binding, VkShaderStageFlags stage, const std::vector<ui32>& sizes, BufferType bufferType = BUFFER_UNIFORM);
		VulkanBindingSet&	addSamplerArrayBinding(ui32 binding, VkShaderStageFlags stage, const std::vector<std::string>& texturePaths, std::vector<TextureType> types);

		ui32											getId() const noexcept { return this->id; }
		const std::vector<std::unique_ptr<BindInfo>>&	getBindingData() const noexcept { return this->bindings; }
		std::vector<VkDescriptorSetLayoutBinding>		getVkBindingData() const noexcept;

	private:
		static ui32 ID_INSTANCE;

		std::vector<std::unique_ptr<BindInfo>>	bindings;
		const ui32								id;
};

class VulkanDescriptorSet;

class VulkanDescriptorSetFactory
{
	public:
		VulkanDescriptorSetFactory() = delete;
		VulkanDescriptorSetFactory(VulkanDevice& vulkanDevice) noexcept : vulkanDevice{vulkanDevice} {}
		~VulkanDescriptorSetFactory() noexcept;
		VulkanDescriptorSetFactory(const VulkanDescriptorSetFactory& other) = delete;
		VulkanDescriptorSetFactory(VulkanDescriptorSetFactory& other) noexcept;
		VulkanDescriptorSetFactory& operator=(const VulkanDescriptorSetFactory& other) = delete;
		VulkanDescriptorSetFactory& operator=(VulkanDescriptorSetFactory& other) = delete;

		VulkanDescriptorSetFactory&	addPoolSize(VkDescriptorType type, ui32 count = 1U);
		VulkanDescriptorSetFactory&	addBufferPoolSize(ui32 count = 1U);
		VulkanDescriptorSetFactory&	addSsboPoolSize(ui32 count = 1U);
		VulkanDescriptorSetFactory&	addSamplerPoolSize(ui32 count = 1U);

		VulkanDescriptorSetFactory&	setPoolFlags(VkDescriptorPoolCreateFlags flags) noexcept;
		VulkanDescriptorSetFactory&	setMaxSets(ui32 count) noexcept;

		VulkanDescriptorSetFactory& createPool();
		VulkanDescriptorSetFactory&	resetPool() noexcept;

		std::unique_ptr<VulkanDescriptorSet>	createDescriptorSet(const VulkanBindingSet& bindings);

	private:
		void	addNewLayout(const VulkanBindingSet& bindings);

		VulkanDevice&				vulkanDevice;
		VkDescriptorPoolCreateFlags	poolFlags{0U};
		ui32						maxSets{0U};

		std::map<VkDescriptorType,ui32>		countTypes{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0U},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0U},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0U},
		};
		VkDescriptorPool						descriptorPool{VK_NULL_HANDLE};
		// descriptor set layouts are linked to a specific binding, the map is used for fast lookup
		// the vector is instead returned to be given to pipelines creation
		std::map<ui32, VkDescriptorSetLayout>	existingLayouts;
		std::vector<VkDescriptorSetLayout>		descriptorSetlayouts;
};


class VulkanDescriptor;
class VulkanBufferDescriptor;
class VulkanSamplerDescriptor;

class VulkanDescriptorSet
{
	public:
		VulkanDescriptorSet() = delete;
		VulkanDescriptorSet(
			VulkanDevice& 			vulkanDevice,
			VkDescriptorSetLayout	descriptorSetLayout,
			VkDescriptorPool		descriptorPool,
			const VulkanBindingSet&	bindings
		);
		~VulkanDescriptorSet() = default;
		VulkanDescriptorSet(const VulkanDescriptorSet& other) = delete;
		VulkanDescriptorSet(VulkanDescriptorSet&& other) = delete;
		VulkanDescriptorSet& operator=(const VulkanDescriptorSet& other) = delete;
		VulkanDescriptorSet& operator=(VulkanDescriptorSet&& other) = delete;

		void	updateDescriptor(ui32 binding, const void* data, ui32 index = 0U) noexcept;
		void	bindSet(VkCommandBuffer commandBuffer, const VulkanPipeline& pipeline, ui32 setIndex) noexcept;

		VkDescriptorSetLayout			getLayout() const noexcept { return this->descriptorSetLayout; };
		const VulkanBufferDescriptor*	getBufferDescriptor(ui32 binding) const noexcept;
		const VulkanSamplerDescriptor*	getSamplerDescriptor(ui32 binding) const noexcept;

	private:
		VkDescriptorSetLayout	descriptorSetLayout{VK_NULL_HANDLE};
		VkDescriptorSet			descriptorSet{VK_NULL_HANDLE};

		std::map<ui32, std::unique_ptr<VulkanDescriptor>>	descriptors{};

		friend class VulkanDescriptorSetFactory;
};

class VulkanDescriptor
{
	public:
		VulkanDescriptor(const BindInfo& binding) : binding{binding.getBinding()} {}
		VulkanDescriptor() = delete;
		virtual ~VulkanDescriptor() {};
		VulkanDescriptor(const VulkanDescriptor& other) = delete;
		VulkanDescriptor(VulkanDescriptor&& other) = default;
		VulkanDescriptor& operator=(const VulkanDescriptor& other) = delete;
		VulkanDescriptor& operator=(VulkanDescriptor&& other) = delete;

		virtual void	update(const void* data, ui32 index = 0U) noexcept = 0;

	protected:
		ui32	binding;
};

class VulkanBufferDescriptor : public VulkanDescriptor
{
	public:
		VulkanBufferDescriptor(const UniformBindInfo& binding, VulkanDevice& vulkanDevice, VkDescriptorSet descriptorSet);

		void	update(const void* data, ui32 index = 0U) noexcept override;

	private:
		std::vector<std::unique_ptr<VulkanBuffer>>	buffers{};
};

class VulkanSamplerDescriptor : public VulkanDescriptor
{
	public:
		VulkanSamplerDescriptor(const SamplerBindInfo& binding, VulkanDevice& vulkanDevice, VkDescriptorSet descriptorSet);

		void	update(const void* data, ui32 index = 0U) noexcept override;

		FontModel	getModelFromText(const std::string& text, const vec2i& origin, ui32 index = 0U, bool isRightAligned = false) const noexcept;

	private:
		std::vector<std::unique_ptr<VulkanTexture>>	textures{};
};

}  // namespace ve
