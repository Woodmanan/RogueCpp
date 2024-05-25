#include "Window.h"
#include "../../../libraries/BearLibTerminal/Include/C/BearLibTerminal.h"

//Lightweight window interface for better control over BearLibTerminal

void Window::Put(int x, int y, int character, Color fg, Color bg)
{
	MatchLayer();
	terminal_color(fg);
	terminal_bkcolor(bg);
	if (x >= 0 && y >= 0 && x < m_rect.w && y < m_rect.h)
	{
		terminal_put(x + m_rect.x, y + m_rect.y, character);
		if (m_renderBackground && m_layer != 0)
		{
			terminal_layer(m_layer - 1);
			terminal_color(bg);
			terminal_put(x + m_rect.x, y + m_rect.y, L'\u2588'); //Unicode for blocking char
		}
	}
}

void Window::PutRect(Rect rect, int character, Color fg, Color bg)
{
	MatchLayer();
	terminal_color(fg);
	terminal_bkcolor(bg);

	short minX = std::max((int) rect.x, 0);
	short maxX = std::min(rect.x + rect.w, (int) m_rect.w);
	short minY = std::max((int) rect.y, 0);
	short maxY = std::min(rect.y + rect.h, (int) m_rect.h);

	for (int x = minX; x < maxX; x++)
	{
		for (int y = minY; y < maxY; y++)
		{
			terminal_put(x + m_rect.x, y + m_rect.y, character);
		}
	}

	if (m_renderBackground && m_layer != 0)
	{
		terminal_layer(m_layer - 1);
		terminal_color(bg);
		for (int x = minX; x < maxX; x++)
		{
			for (int y = minY; y < maxY; y++)
			{
				terminal_put(x + m_rect.x, y + m_rect.y, L'\u2588');
			}
		}
	}
}

void Window::Clear()
{
	ClearRect({0, 0, m_rect.w, m_rect.h}, Color(0, 0, 0, 0));
}

void Window::ClearRect(Rect rect, Color bg)
{
	MatchLayer();
	short minX = std::max((int)rect.x, 0);
	short maxX = std::min(rect.x + rect.w, (int)m_rect.w);
	short minY = std::max((int)rect.y, 0);
	short maxY = std::min(rect.y + rect.h, (int)m_rect.h);
	terminal_clear_area(minX, minY, maxX - minX, maxY - minY);
	if (m_renderBackground && m_layer != 0)
	{
		terminal_layer(m_layer - 1);
		terminal_color(bg);
		for (int x = minX; x < maxX; x++)
		{
			for (int y = minY; y < maxY; y++)
			{
				terminal_put(x + m_rect.x, y + m_rect.y, L'\u2588');
			}
		}
	}

}

void Window::MatchLayer()
{
	if (terminal_state(TK_LAYER) != m_layer)
	{
		terminal_layer(m_layer);
	}
}