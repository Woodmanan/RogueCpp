#pragma once
#include "Debug/Debug.h"
#include "Data/Resources.h"
#include "Data/SaveManager.h"
#include "Core/CoreDataTypes.h"

struct RogueImage
{
	int m_width = 0;
	int m_height = 0;
	int m_textureChannels = 0;
	std::vector<unsigned char> m_pixels;

	int GetLineSize() const;
	int GetByteSize() const;
	uchar* GetScanline(int row);
	uchar* GetPixel(int row, int col);

	void InsertImage(RogueImage& image, Vec2 position);
};

class ImageManager
{
public:
	ImageManager();

	RogueImage* CreateEmptyImage(int x, int y, int texChannels);
	RogueImage* PadToSize(RogueImage& image, Vec2 size);
	RogueImage* CreateAtlas(std::vector<RogueImage*>& images, std::vector<Vec2>& positions, int size);

	static ImageManager* Get() { return manager; }

private:
	static ImageManager* manager;
};

namespace RogueResources
{
	void PackImage(PackContext& packContext);
	std::shared_ptr<void> LoadImage(LoadContext& loadContext);
}

namespace RogueSaveManager
{
	template<typename Stream>
	void Serialize(Stream& stream, RogueImage& value)
	{
		Write(stream, "Width", value.m_width);
		Write(stream, "Width", value.m_height);
		Write(stream, "Tex Channels", value.m_textureChannels);
		WriteRawBytes(stream, "Pixels", value.m_pixels);
	}

	template<typename Stream>
	void Deserialize(Stream& stream, RogueImage& value)
	{
		Read(stream, "Width", value.m_width);
		Read(stream, "Width", value.m_height);
		Read(stream, "Tex Channels", value.m_textureChannels);
		ReadRawBytes(stream, "Pixels", value.m_pixels);
	}
}