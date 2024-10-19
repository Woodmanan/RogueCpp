#include "ImageManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "Debug/Profiling.h"

ImageManager* ImageManager::manager = new ImageManager();

ImageManager::ImageManager()
{
	RogueResources::Register("Image", GetMember(this, &ImageManager::PackImage), GetMember(this, &ImageManager::LoadImage));
}

void ImageManager::PackImage(RogueResources::PackContext& packContext)
{
	ROGUE_PROFILE_SECTION("ImageManager::Pack");
	std::ifstream stream(packContext.source, std::ios::binary | std::ios::ate);
	std::streamsize size = stream.tellg();
	stream.seekg(0, std::ios::beg);

	std::vector<unsigned char> buffer(size);

	if (stream.read((char*) buffer.data(), size))
	{
		RogueResources::OpenWritePackFile(packContext.destination);
		RogueSaveManager::WriteAsBuffer("Buffer", buffer);
		RogueSaveManager::CloseWriteSaveFile();
	}
}

std::shared_ptr<void> ImageManager::LoadImage(RogueResources::LoadContext& loadContext)
{
	ROGUE_PROFILE_SECTION("ImageManager::Load");
	std::vector<unsigned char> buffer;

	RogueResources::OpenReadPackFile(loadContext.source);
	RogueSaveManager::ReadAsBuffer("Buffer", buffer);
	RogueSaveManager::CloseReadSaveFile();

	RogueImage* image = new RogueImage();

	stbi_uc* pixels = stbi_load_from_memory((stbi_uc*)buffer.data(), buffer.size(), &image->m_width, &image->m_height, &image->m_textureChannels, STBI_rgb_alpha);
	ASSERT(pixels != nullptr);

	size_t size = image->m_width * image->m_height * 4;
	image->m_pixels.resize(size);
	for (size_t pix = 0; pix < size; pix++)
	{
		image->m_pixels[pix] = pixels[pix];
	}
	// TODO - Make memcpy? Not working for some reason

	stbi_image_free(pixels);

	return std::shared_ptr<RogueImage>(image);
}

namespace RogueSaveManager
{
	void Serialize(RogueImage& value)
	{
		AddOffset();
		Write("Width", value.m_width);
		Write("Width", value.m_height);
		Write("Tex Channels", value.m_textureChannels);
		WriteAsBuffer("Pixels", value.m_pixels);
		RemoveOffset();
	}

	void Deserialize(RogueImage& value)
	{
		AddOffset();
		Read("Width", value.m_width);
		Read("Width", value.m_height);
		Read("Tex Channels", value.m_textureChannels);
		ReadAsBuffer("Pixels", value.m_pixels);
		RemoveOffset();
	}
}