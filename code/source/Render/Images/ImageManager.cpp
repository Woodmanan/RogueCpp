#include "ImageManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "Debug/Profiling.h"
#include "rectpack2D/src/finders_interface.h"

using namespace rectpack2D;

ImageManager* ImageManager::manager = new ImageManager();

int RogueImage::GetLineSize() const
{
	return m_width * m_textureChannels;
}

int RogueImage::GetByteSize() const
{
	return m_height * GetLineSize();
}

uchar* RogueImage::GetScanline(int row)
{
	ASSERT(row >= 0 && row < m_height);
	int rowSize = m_width * m_textureChannels;
	return m_pixels.data() + (rowSize * row);
}

uchar* RogueImage::GetPixel(int row, int col)
{
	uchar* scanline = GetScanline(row);
	return scanline + (col * m_textureChannels);
}

void RogueImage::InsertImage(RogueImage& image, Vec2 position)
{
	ASSERT(m_textureChannels == image.m_textureChannels); //We don't support copying from different channels yet.
	ASSERT(position.x >= 0 && position.y >= 0);
	ASSERT(position.x + image.m_width <= m_width && position.y + image.m_height <= m_height);

	for (int imgY = 0; imgY < image.m_height; imgY++)
	{
		for (int imgX = 0; imgX < image.m_width; imgX++)
		{
			memcpy(GetPixel(imgY + position.y, imgX + position.x), image.GetPixel(imgY, imgX), m_textureChannels);
		}
	}
}

ImageManager::ImageManager()
{

}

RogueImage* ImageManager::CreateEmptyImage(int x, int y, int texChannels)
{
	RogueImage* image = new RogueImage();
	image->m_width = x;
	image->m_height = y;
	image->m_textureChannels = texChannels;
	image->m_pixels.resize(x * y * texChannels);
	return image;
}

RogueImage* ImageManager::PadToSize(RogueImage& image, Vec2 size)
{
	size = Vec2(std::max<short>(size.x, image.m_width), std::max<short>(size.y, image.m_height));
	Vec2 offset = size - Vec2(image.m_width, image.m_height);
	Vec2 leftPad = offset / 2;

	RogueImage* copy = CreateEmptyImage(size.x, size.y, image.m_textureChannels);

	for (int j = 0; j < image.m_height; j++)
	{
		for (int i = 0; i < image.m_width; i++)
		{
			*copy->GetPixel(j + leftPad.y, i + leftPad.x) = *image.GetPixel(j, i);
		}
	}

	return copy;
}

RogueImage* ImageManager::CreateAtlas(std::vector<RogueImage*>& images, std::vector<Vec2>& positions, int size)
{
	constexpr bool allow_flip = false;
	const auto runtime_flipping_mode = flipping_option::DISABLED;
	using spaces_type = rectpack2D::empty_spaces<allow_flip, default_empty_spaces>;
	using rect_type = output_rect_t<spaces_type>;
	auto report_successful = [](rect_type&) {
		return callback_result::CONTINUE_PACKING;
	};

	auto report_unsuccessful = [](rect_type&) {
		return callback_result::ABORT_PACKING;
	};

	const auto discard_step = -4;

	std::vector<rect_type> rectangles;
	for (const RogueImage* img : images)
	{
		rectangles.emplace_back(rect_xywh(0, 0, img->m_width, img->m_height));
	}

	const auto result_size = find_best_packing<spaces_type>(
		rectangles,
		make_finder_input(
			size,
			discard_step,
			report_successful,
			report_unsuccessful,
			runtime_flipping_mode
		)
	);

	RogueImage* image = CreateEmptyImage(size, size, 4);
	for (int i = 0; i < rectangles.size(); i++)
	{
		rect_type rect = rectangles[i];
		positions.push_back(Vec2(rect.x, rect.y));
		image->InsertImage(*images[i], Vec2(rect.x, rect.y));
	}

	return image;
}

namespace RogueResources
{
	void PackImage(PackContext& packContext)
	{
		ROGUE_PROFILE_SECTION("PackImage");
		std::ifstream stream(packContext.source, std::ios::binary | std::ios::ate);
		std::streamsize size = stream.tellg();
		stream.seekg(0, std::ios::beg);

		std::vector<unsigned char> buffer(size);

		if (stream.read((char*)buffer.data(), size))
		{
			OpenWritePackFile(packContext.destination, packContext.header);
			RogueSaveManager::WriteAsBuffer("Buffer", buffer);
			RogueSaveManager::CloseWriteSaveFile();
		}
	}

	std::shared_ptr<void> LoadImage(LoadContext& loadContext)
	{
		ROGUE_PROFILE_SECTION("LoadImage");
		std::vector<unsigned char> buffer;

		OpenReadPackFile(loadContext.source);
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