#include "FontManager.h"
#include "Debug/Profiling.h"

FontManager* FontManager::manager = new FontManager();

RogueFont::RogueFont(FT_Library m_library, std::vector<unsigned char>& buffer)
{
	m_buffer.insert(m_buffer.end(), buffer.begin(), buffer.end());
	FT_Error error = FT_New_Memory_Face(m_library, m_buffer.data(), m_buffer.size(), 0, &m_face);
	ASSERT(!error);
}

RogueFont::~RogueFont()
{
	FT_Done_Face(m_face);
	m_buffer.clear();
}

bool RogueFont::SetSize(uint xSize, uint ySize)
{
	return !FT_Set_Pixel_Sizes(m_face, xSize, ySize);
}

bool RogueFont::HasActiveCharmap()
{
	return FT_Get_Charmap_Index(m_face->charmap) != -1;
}

bool RogueFont::SelectCharmap(FT_Encoding encoding)
{
	return !FT_Select_Charmap(m_face, encoding);
}

FT_ULong RogueFont::GetFirstCharCode()
{
	ASSERT(HasActiveCharmap());
	FT_UInt glyphIndex;
	return FT_Get_First_Char(m_face, &glyphIndex);
}

bool RogueFont::HasNextCharCode(FT_ULong current)
{
	FT_UInt glyphIndex;
	FT_ULong charcode = FT_Get_Next_Char(m_face, current, &glyphIndex);
	return glyphIndex != 0;
}

FT_ULong RogueFont::GetNextCharCode(FT_ULong current)
{
	ASSERT(HasActiveCharmap());
	ASSERT(HasNextCharCode(current));
	FT_UInt glyphIndex;
	return FT_Get_Next_Char(m_face, current, &glyphIndex);
}

const RogueImage& RogueFont::LoadGlyph(FT_ULong glyph)
{
	ROGUE_PROFILE_SECTION("RogueFont::LoadGlyph");
	if (!m_glyphs.contains(glyph))
	{
		FT_Error error = FT_Load_Char(m_face, glyph, FT_LOAD_RENDER);
		ASSERT(!error);

		if (m_face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
		{
			error = FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_NORMAL); //Could be set to FT_RENDER_MODE_MONO for solid black/white
			ASSERT(!error);
		}

		AddImageForGlyph(glyph, m_face->glyph->bitmap);
	}

	return m_glyphs[glyph];
}

void RogueFont::AddImageForGlyph(FT_ULong glyph, FT_Bitmap& bitmap)
{
	RogueImage image;
	image.m_width = bitmap.width;
	image.m_height = bitmap.rows;
	image.m_textureChannels = 4;
	image.m_pixels.resize(image.GetByteSize());

	unsigned char* scanline = bitmap.buffer;

	for (int row = 0; row < bitmap.rows; row++)
	{
		SetRow(image, row, scanline, bitmap.pixel_mode);
		scanline = scanline + bitmap.pitch;
	}

	m_glyphs[glyph] = image;
}

void RogueFont::SetRow(RogueImage& image, int row, unsigned char* scanline, unsigned char pixelMode)
{
	unsigned char* imageScanline = image.GetScanline(row);
	switch (pixelMode)
	{
	case FT_PIXEL_MODE_GRAY:
		for (int i = 0; i < image.m_width; i++)
		{
			unsigned char value = scanline[i];
			unsigned char* pixel = imageScanline + (i * image.m_textureChannels);
			pixel[0] = value;
			pixel[1] = 0;
			pixel[2] = 0;
			pixel[3] = value;
		}
		break;
	case FT_PIXEL_MODE_BGRA:
		memcpy(image.GetScanline(row), scanline, image.GetLineSize());
		break;
	default:
		HALT(); //Unsupported type!
	}
}

FontManager::FontManager()
{
	FT_Error error = FT_Init_FreeType(&m_library);
	ASSERT(!error);
	RogueResources::Register("Font", GetMember(this, &FontManager::PackFont), GetMember(this, &FontManager::LoadFont));
}

FontManager::~FontManager()
{
	FT_Done_FreeType(m_library);
}

void FontManager::PackFont(RogueResources::PackContext& packContext)
{
	ROGUE_PROFILE_SECTION("FontManager::Pack");
	std::ifstream stream(packContext.source, std::ios::binary | std::ios::ate);
	std::streamsize size = stream.tellg();
	stream.seekg(0, std::ios::beg);

	std::vector<unsigned char> buffer(size);

	if (stream.read((char*)buffer.data(), size))
	{
		RogueResources::OpenWritePackFile(packContext.destination);
		RogueSaveManager::WriteAsBuffer("Buffer", buffer);
		RogueSaveManager::CloseWriteSaveFile();
	}
}

std::shared_ptr<void> FontManager::LoadFont(RogueResources::LoadContext& loadContext)
{
	ROGUE_PROFILE_SECTION("FontManager::Load");
	std::vector<unsigned char> buffer;

	RogueResources::OpenReadPackFile(loadContext.source);
	RogueSaveManager::ReadAsBuffer("Buffer", buffer);
	RogueSaveManager::CloseReadSaveFile();

	RogueFont* font = new RogueFont(m_library, buffer);
	font->SelectCharmap(FT_ENCODING_UNICODE);

	return std::shared_ptr<RogueFont>(font);
}