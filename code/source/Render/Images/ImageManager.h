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