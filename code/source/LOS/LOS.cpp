#include "LOS.h"
#include <algorithm>

void View::SetRadius(int radius)
{
	if (m_radius != radius)
	{
		m_radius = radius;
		int diameter = (2 * radius) + 1;
		int numTiles = diameter * diameter;
		m_locations.resize(numTiles);
		m_visibility.resize(numTiles);
	}
}

int View::GetIndexByLocal(int x, int y)
{
	ASSERT(x >= -m_radius && x <= m_radius);
	ASSERT(y >= -m_radius && y <= m_radius);

	return (x + m_radius) + (y + m_radius) * (2 * m_radius + 1);
}

void View::ResetAt(Location location)
{
	std::fill(m_locations.begin(), m_locations.end(), Location());
	std::fill(m_visibility.begin(), m_visibility.end(), false);

	m_locations[GetIndexByLocal(0, 0)] = location;
	m_visibility[GetIndexByLocal(0, 0)] = true;
}

void View::Mark(int x, int y, bool value)
{
	m_visibility[GetIndexByLocal(0, 0)] = value;
}

//Does a LOT of the heavy lifting for this algorithm.
//Gets the most sensible parent for the local location,
//allowing the algorithm to propagate out in non-euclidean space
Vec2 View::GetSensibleParent(int x, int y)
{
	//Move everything to one quadrant - we'll undo it later, but this makes the math consistent
	int absX = std::abs(x);
	int absY = std::abs(y);

	//Int-only version of 2D lerp
	int max = std::max(std::abs(x), std::abs(y));
	int newX = IntLerp(0, absX, max - 1, max);
	int newY = IntLerp(0, absY, max - 1, max);

	//Confirm that we've moved in a direction, otherwise this math is wrong.
	ASSERT(newX < absX || newY < absY);

	if (x < 0) { newX = -newX; }
	if (y < 0) { newY = -newY; }

	return Vec2(newX, newY);
}

/*
 * Builds out the local-space graph of locations, which can then
 * be iterated upon by the shadowcasting algorithm.
 */
void View::BuildLocalSpace()
{
	for (int r = 1; r <= m_radius; r++)
	{
		//Classic box algorithm to collect points.
		for (int x = -r; x <= r; x++)
		{
			BuildLocalSpaceTile(x, r);
			BuildLocalSpaceTile(x, -r);
		}

		for (int y = -r + 1; y <= r - 1; y++)
		{
			BuildLocalSpaceTile(r, y);
			BuildLocalSpaceTile(-r, y);
		}
	}
}

void View::BuildLocalSpaceTile(int x, int y)
{
	Vec2 localParent = GetSensibleParent(x, y);
	Vec2 offset = Vec2(x, y) - localParent;
	Location parent = GetLocationLocal(localParent.x, localParent.y);
	m_locations[GetIndexByLocal(x, y)] = parent.Traverse(offset.x, offset.y);
}

Location View::GetLocationLocal(int x, int y)
{
	return m_locations[GetIndexByLocal(x, y)];
}

bool View::GetVisibilityLocal(int x, int y)
{
	return m_visibility[GetIndexByLocal(x, y)];
}

int IntLerp(int a, int b, int numerator, int denominator)
{
	int offset = (b - a);

	int scaledOffset = (offset * numerator + (denominator / 2)) / denominator;

	return a + scaledOffset;
}

namespace LOS
{
	void Calculate(View& view, Location location)
	{
		view.ResetAt(location);
		view.BuildLocalSpace();
	}
}