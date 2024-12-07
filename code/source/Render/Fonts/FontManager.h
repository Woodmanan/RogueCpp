#pragma once
#include "Core/CoreDataTypes.h"
#include "Debug/Debug.h"
#include "Data/Resources.h"
#include "Data/SaveManager.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include "Render/Images/ImageManager.h" 
#include "glm/glm.hpp"

class RogueFont
{
public:
	const int atlasSize = 512;

	RogueFont(FT_Library m_library, std::vector<unsigned char>& buffer);
	virtual ~RogueFont();

	FT_Face m_face;
	std::map<FT_ULong, RogueImage> m_glyphs;

	bool SetSize(uint xSize, uint ySize);
	bool HasActiveCharmap();
	bool SelectCharmap(FT_Encoding encoding);
	FT_ULong GetFirstCharCode();
	bool HasNextCharCode(FT_ULong current);
	FT_ULong GetNextCharCode(FT_ULong current);
	const RogueImage& LoadGlyph(FT_ULong glyph);

	bool CreateAtlas();
	RogueImage* GetAtlas() { return m_atlas; }
	int GetIndexForGlyph(FT_ULong glyph);

	//Atlas variables
	std::map<FT_ULong, int> m_glyphIndices;
	std::vector<glm::vec2> m_uvStarts;
	std::vector<glm::vec2> m_uvEnds;
	std::vector<float> m_scaling;

private:
	void AddImageForGlyph(FT_ULong glyph, FT_Bitmap& bitmap);
	void SetRow(RogueImage& image, int row, unsigned char* scanline, unsigned char pixelMode);
	std::vector<unsigned char> m_buffer;
	RogueImage* m_atlas = nullptr;
	Vec2 m_size;
};

class FontManager
{
public:
	FontManager();
	~FontManager();
	void PackFont(RogueResources::PackContext& packContext);
	std::shared_ptr<void> LoadFont(RogueResources::LoadContext& loadContext);

	static FontManager* Get() { return manager; }

private:
	static FontManager* manager;
	FT_Library m_library;
};