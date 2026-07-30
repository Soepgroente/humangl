#pragma once

#include "VulkanObject.hpp"

#include "stb_truetype.h"


namespace ve {

struct ImageInfo
{
	unsigned char*	imageData;
	i32			width;
	i32			height;
	i32			channels;
};

struct FontInfo
{
	unsigned char*				fontData;
	std::vector<unsigned char>	fileContent;
	i32							width;
	i32							height;
	stbtt_bakedchar 			cdata[128];
	stbtt_fontinfo				basicFontInfo;
};

struct FontModel
{
	std::shared_ptr<VulkanModel> background;
	std::shared_ptr<VulkanModel> text;
};

std::unique_ptr<ImageInfo>		loadImage(const std::string& imagePath);
std::unique_ptr<FontInfo>		loadFont(const std::string& fontPath, f32 fontSize, VkExtent2D sizeTexture);

class VulkanTexture
{
	public:

	VulkanTexture() = delete;
	VulkanTexture(VulkanDevice& device, const std::string& filePath, TextureType = TEXTURE_PLAIN);
	~VulkanTexture();
	VulkanTexture(const VulkanTexture& other) = delete;
	VulkanTexture(VulkanTexture&&);
	VulkanTexture& operator=(const VulkanTexture& other) = delete;

	VkDescriptorImageInfo	getDescriptorImageInfo() const noexcept;
	FontModel				getModelFromText(const std::string& text, const vec2i& origin, bool isRightAligned = false) const noexcept;

	static constexpr ui32 defaultSizeFont = 32U;
	static constexpr VkExtent2D defaultSizeFontTexture = VkExtent2D{512U, 512U};
	static constexpr ui32 fontPadding = 5U;

	private:

	void	createTextureImage();
	void	createTextureImageView();
	void	createTextureSampler();

	VulkanDevice&	device;
	TextureType		type;

	std::unique_ptr<ImageInfo>	imageInfo;
	std::unique_ptr<FontInfo>	fontInfo;
	VkImageCreateInfo			info{};
	VkDeviceSize				nPixels{0UL};
	VkDeviceSize				sizeOfPixel{STBTT_UNICODE_EID_UNICODE_2_0_FULL};

	VkImage			textureImage{VK_NULL_HANDLE};
	VkDeviceMemory	textureImageMemory{VK_NULL_HANDLE};
	VkImageView		textureImageView{VK_NULL_HANDLE};
	VkSampler		textureSampler{VK_NULL_HANDLE};

};

} // namespace ve
