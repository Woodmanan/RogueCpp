#pragma once
#include "../../Core/CoreDataTypes.h"

union Rect
{
	struct
	{
		short x;
		short y;
		short w;
		short h;
	};
	struct
	{
		Vec2 position;
		Vec2 size;
	};
};

class Window
{
public:
	Window(Rect rect) : m_rect(rect) {}
	Window(Rect rect, int layer) : m_rect(rect), m_layer(layer) {}

	int m_layer = 0;
	Rect m_rect;
	bool m_renderBackground = true;

	void Put(int x, int y, int character, Color fg, Color bg);
	void PutRect(Rect rect, int character, Color fg, Color bg);

	void Clear();
	void ClearRect(Rect rect, Color bg);

	void MatchLayer();
};
