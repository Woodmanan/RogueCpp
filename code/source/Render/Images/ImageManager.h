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
	void Serialize(RogueImage& value);
	void Deserialize(RogueImage& value);
}