#include "Window.h"
#include "Render/Terminal.h"
#include <algorithm>
#include "Render/RenderTools.h"
#include "Debug/Profiling.h"

//Lightweight window interface for better control over BearLibTerminal

Window::Window(string id, Anchors anchors, Window* parent)
{
	m_id = id;
	m_anchors = anchors;
	m_rect = Rect({ 0,0,0,0 });
	SetParent(parent);
}

void Window::UpdateLayout(const Rect& parentRect, const int parentLayer)
{
	Rect newRect = GenerateRect(parentRect);
	m_layer = parentLayer + m_layerOffset;
	if (newRect != m_rect)
	{
		m_rect = newRect;
		OnSizeUpdated();
	}

	for (Window* child : m_children)
	{
		child->UpdateLayout(m_rect, m_layer);
	}
}

Rect Window::GenerateRect(const Rect& parentRect)
{
	short startX = parentRect.x + ((short)(parentRect.w * m_anchors.minX)) + m_anchors.minXOffset;
	short endX   = parentRect.x + ((short)(parentRect.w * m_anchors.maxX)) + m_anchors.maxXOffset;
	short startY = parentRect.y + ((short)(parentRect.h * m_anchors.minY)) + m_anchors.minYOffset;
	short endY   = parentRect.y + ((short)(parentRect.h * m_anchors.maxY)) + m_anchors.maxYOffset;

	return Rect({startX, startY, short(endX - startX), short(endY - startY) });
}

void Window::SetParent(Window* parent)
{
	if (m_parent != nullptr)
	{
		m_parent->RemoveChild(this);
	}

	if (parent != nullptr)
	{
		parent->AddChild(this);
	}

	m_parent = parent;
}

void Window::AddChild(Window* child)
{
	ASSERT(find(m_children.begin(), m_children.end(), child) == m_children.end());
	m_children.push_back(child);
}

void Window::RemoveChild(Window* child)
{
	auto it = find(m_children.begin(), m_children.end(), child);
	ASSERT(it != m_children.end());
	m_children.erase(it);
}

void Window::RenderContent(float deltaTime)
{
	ROGUE_PROFILE;
	OnDrawContent(deltaTime);
	for (Window* child : m_children)
	{
		child->RenderContent(deltaTime);
	}
}

void Window::Put(int x, int y, int character, Color fg, Color bg)
{
	MatchLayer();
	terminal_color(fg);
	terminal_bkcolor(bg);
	if (x >= 0 && y >= 0 && x < m_rect.w && y < m_rect.h)
	{
		terminal_put(x + m_rect.x, y + m_rect.y, character);
	}
}

void Window::PutRect(Rect rect, int character, Color fg, Color bg)
{
	MatchLayer();
	terminal_color(fg);
	terminal_bkcolor(bg);

	short windowX = std::max((int)rect.x, 0);
	short windowXSize = std::min((int)rect.w, m_rect.w - windowX);
	short windowY = std::max((int)rect.y, 0);
	short windowYSize = std::max((int)rect.h, m_rect.h - windowY);

	for (int x = windowX; x < windowX + windowXSize; x++)
	{
		for (int y = windowY; y < windowY + windowYSize; y++)
		{
			terminal_put(x + m_rect.x, y + m_rect.y, character);
		}
	}
}

void Window::DrawRect(Rect rect, Color fg)
{
	MatchLayer();
	terminal_color(fg);
	short windowX = std::max((int)rect.x, 0);
	short windowXSize = std::min((int)rect.w, m_rect.w - windowX);
	short windowY = std::max((int)rect.y, 0);
	short windowYSize = std::max((int)rect.h, m_rect.h - windowY);

	RenderTools::DrawRect({ short(m_rect.x + windowX), short(m_rect.y + windowY), short(windowXSize), short(windowYSize)});
}

void Window::Clear()
{
	ClearRect({0, 0, m_rect.w, m_rect.h}, Color(0, 0, 0, 0));
}

void Window::ClearRect(Rect rect, Color bg)
{
	MatchLayer();
	terminal_bkcolor(bg);

	short windowX = std::max((int)rect.x, 0);
	short windowXSize = std::min((int) rect.w, m_rect.w - windowX);
	short windowY = std::max((int)rect.y, 0);
	short windowYSize = std::max((int)rect.h, m_rect.h - windowY);
	terminal_clear_area(m_rect.x + windowX, m_rect.y + windowY, windowXSize, windowYSize);
}

void Window::MatchLayer()
{
	terminal_layer(m_layer);
}

void Window::RenderSelection()
{
	Color fg = Color(55, 55, 55);
	Color bg = Color(0, 0, 0);

	if (ContainsMouse())
	{
		fg = Color(140, 140, 140);
	}

	PutRect({0,0, m_rect.w, m_rect.h}, 'X', fg, bg);

	for (Window* child : m_children)
	{
		child->RenderSelection();
	}
}

void Window::RenderAnchors()
{
	Rect parentRect{ 0,0,0,0 };
	if (m_parent)
	{
		parentRect = m_parent->m_rect;
	}
	else
	{
		parentRect = Rect({ 0,0, short(terminal_width()), short(terminal_height())});
	}

	short startX = parentRect.x + ((short)(parentRect.w * m_anchors.minX));
	short endX = parentRect.x + ((short)(parentRect.w * m_anchors.maxX)) - 1;
	short startY = parentRect.y + ((short)(parentRect.h * m_anchors.minY));
	short endY = parentRect.y + ((short)(parentRect.h * m_anchors.maxY)) - 1;


	RenderTools::DrawBresenham(m_rect.x, m_rect.y,startX, startY);
	RenderTools::DrawBresenham(m_rect.x + m_rect.w - 1, m_rect.y, endX, startY);
	RenderTools::DrawBresenham(m_rect.x, m_rect.y + m_rect.h - 1, startX, endY);
	RenderTools::DrawBresenham(m_rect.x + m_rect.w - 1, m_rect.y + m_rect.h - 1, endX, endY);

	DrawRect({0, 0, m_rect.w, m_rect.h}, Color(255, 0, 255));
}

bool Window::ContainsMouse()
{
	int mouseX = 0;//terminal_state(TK_MOUSE_X);
	int mouseY = 0;// terminal_state(TK_MOUSE_Y);

	return (mouseX >= m_rect.x && mouseX < (m_rect.x + m_rect.w) &&
		    mouseY >= m_rect.y && mouseY < (m_rect.y + m_rect.h));
}

Window* Window::GetSelectedWindow()
{
	if (ContainsMouse())
	{
		Window* best = this;
		for (Window* child : m_children)
		{
			if (child->ContainsMouse() && child->m_layer > best->m_layer)
			{
				best = child->GetSelectedWindow();
			}
		}

		return best;
	}

	return nullptr;
}