#pragma once
#include "../../Core/CoreDataTypes.h"
#include <vector>

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

	bool operator==(const Rect& rhs) const
	{
		return (x == rhs.x) && (y == rhs.y) && (w == rhs.w) && (h == rhs.h);
	}

	bool operator!=(const Rect& rhs) const
	{
		return !(*this == rhs);
	}
};

struct Anchors
{
	float minX;
	float maxX;
	float minY;
	float maxY;
	short minXOffset;
	short maxXOffset;
	short minYOffset;
	short maxYOffset;
};

using namespace std;

class Window
{
public:
	Window(string id, Anchors anchors, Window* parent = nullptr);

	string m_id;

	//Layout
	Rect m_rect = Rect({ 0,0,0,0 });
	Anchors m_anchors;

	//Tree Structure
	Window* m_parent = nullptr;
	vector<Window*> m_children;

	//Render controls
	int m_layer = -1;
	int m_layerOffset = 1;
	bool m_renderBackground = true;

public:
	void UpdateLayout(const Rect& parentRect, const int parentLayer = -1);
	Rect GenerateRect(const Rect& parentRect);

	void SetParent(Window* parent);
	void AddChild(Window* child);
	void RemoveChild(Window* child);

	virtual void OnSizeUpdated() {}
	void RenderContent(float deltaTime);
	virtual void OnDrawContent(float deltaTime) {}
	
	//Drawing
	void Put(int x, int y, int character, Color fg, Color bg);
	void PutRect(Rect rect, int character, Color fg, Color bg);
	void DrawRect(Rect rect, Color fg);
	void Clear();
	void ClearRect(Rect rect, Color bg);
	void MatchLayer();
	void RenderSelection();
	void RenderAnchors();

	//Selection
	bool ContainsMouse();
	Window* GetSelectedWindow();
};
