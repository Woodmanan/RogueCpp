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
	m_visibility[GetIndexByLocal(x, y)] = value;
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

	int scaledOffset = IntDivisionCeil(offset * numerator, denominator);

	//int scaledOffset = (offset * numerator + (denominator / 2)) / denominator;

	return a + scaledOffset;
}

namespace LOS
{
	void Calculate(View& view, Location location)
	{
		view.ResetAt(location);
		view.BuildLocalSpace();

		CalculateQuadrant(view, West);
		CalculateQuadrant(view, East);
		CalculateQuadrant(view, North);
		CalculateQuadrant(view, South);
	}

	void CalculateQuadrant(View& view, Direction direction)
	{
		Row start = Row(1, Fraction(-1, 1), Fraction(1, 1));
		Scan(view, direction, start);
	}

	void Scan(View& view, Direction direction, Row& row)
	{
		if (row.m_depth > view.GetRadius()) { return; }

		int minCol = row.GetMinCol();
		int maxCol = row.GetMaxCol();

		Location prevTile = Location();

		for (int col = minCol; col <= maxCol; col++)
		{
			Location tile = GetTile(view, direction, col, row.m_depth);
			bool prevWall = IsWall(prevTile);

			if (IsWall(tile) || IsSymmetric(row, col))
			{
				Reveal(view, direction, col, row.m_depth);
			}
			if (IsWall(prevTile) && IsFloor(tile))
			{
				row.m_startSlope = Slope(col, row.m_depth);
			}
			if (IsFloor(prevTile) && IsWall(tile))
			{
				//Move to next row!
				Row nextRow = Row(row.m_depth + 1, row.m_startSlope, Slope(col, row.m_depth));
				Scan(view, direction, nextRow);
			}

			prevTile = tile;
		}

		if (IsFloor(prevTile))
		{
			//Scan next row!
			Row nextRow = Row(row.m_depth + 1, row.m_startSlope, row.m_endSlope);
			Scan(view, direction, nextRow);
		}
	}

	Location GetTile(View& view, Direction direction, int col, int row)
	{
		int x = 0;
		int y = 0;
		
		switch (direction)
		{
		case North:
			x = col;
			y = row;
			break;
		case East:
			x = row;
			y = col;
			break;
		case South:
			x = col;
			y = -row;
			break;
		case West:
			x = -row;
			y = col;
			break;
		}

		return view.GetLocationLocal(x, y);
	}

	bool IsSymmetric(Row& row, int col)
	{
		bool withinLower = (col * row.m_startSlope.m_denominator >= row.m_depth * row.m_startSlope.m_numerator);
		bool withinUpper = (col * row.m_endSlope.m_denominator <= row.m_depth * row.m_endSlope.m_numerator);
		return withinLower && withinUpper;
	}

	bool IsWall(Location location)
	{
		return location.GetValid() && location->m_backingTile->m_blocksVision;
	}

	bool IsFloor(Location location)
	{
		return location.GetValid() && !location->m_backingTile->m_blocksVision;
	}

	void Reveal(View& view, Direction direction, int col, int row)
	{
		int x = 0;
		int y = 0;

		switch (direction)
		{
		case North:
			x = col;
			y = row;
			break;
		case East:
			x = row;
			y = col;
			break;
		case South:
			x = col;
			y = -row;
			break;
		case West:
			x = -row;
			y = col;
			break;
		}

		view.Mark(x, y, true);
	}

	Fraction Slope(int col, int row)
	{
		return Fraction(2 * col - 1, 2 * row);
	}
}