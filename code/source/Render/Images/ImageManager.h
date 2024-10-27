#pragma once
#include "Debug/Debug.h"
#include "Data/Resources.h"
#include "Data/SaveManager.h"

struct RogueImage
{
	int m_width = 0;
	int m_height = 0;
	int m_textureChannels = 0;
	std::vector<unsigned char> m_pixels;

	int GetLineSize() const
	{
		return m_width * m_textureChannels;
	}

	int GetByteSize() const
	{
		return m_height * GetLineSize();
	}

	unsigned char* GetScanline(int row)
	{
		int rowSize = m_width * m_textureChannels;
		return m_pixels.data() + (rowSize * row);
	}
};

class ImageManager
{
public:
	ImageManager();
	void PackImage(RogueResources::PackContext& packContext);
	std::shared_ptr<void> LoadImage(RogueResources::LoadContext& loadContext);

	ImageManager* Get() { return manager; }

private:
	static ImageManager* manager;
};

namespace RogueSaveManager
{
	void Serialize(RogueImage& value);
	void Deserialize(RogueImage& value);
}