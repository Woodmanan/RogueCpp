#pragma once
#include "Core/CoreDataTypes.h"
#include "Debug/Debug.h"
#include "Data/Resources.h"
#include "Data/SaveManager.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include "Render/Images/ImageManager.h" 

class RogueFont
{
public:
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

private:
	void AddImageForGlyph(FT_ULong glyph, FT_Bitmap& bitmap);
	void SetRow(RogueImage& image, int row, unsigned char* scanline, unsigned char pixelMode);
	std::vector<unsigned char> m_buffer;
};

class FontManager
{
public:
	FontManager();
	~FontManager();
	void PackFont(RogueResources::PackContext& packContext);
	std::shared_ptr<void> LoadFont(RogueResources::LoadContext& loadContext);

	FontManager* Get() { return manager; }

private:
	static FontManager* manager;
	FT_Library m_library;
};