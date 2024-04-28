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

int View::GetIndexByLocal(int x, int y) const
{
	ASSERT(x >= -m_radius && x <= m_radius);
	ASSERT(y >= -m_radius && y <= m_radius);

	return (x + m_radius) + (y + m_radius) * (2 * m_radius + 1);
}

void View::ResetAt(Location location)
{
	std::fill(m_locations.begin(), m_locations.end(), Location());
	std::fill(m_visibility.begin(), m_visibility.end(), 0);

	m_locations[GetIndexByLocal(0, 0)] = location;
	m_visibility[GetIndexByLocal(0, 0)] = 1;
}

void View::Mark(int x, int y, uchar value)
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

Location View::GetLocationLocal(int x, int y) const
{
	return m_locations[GetIndexByLocal(x, y)];
}

bool View::GetVisibilityLocal(int x, int y) const
{
	return m_visibility[GetIndexByLocal(x, y)] > 0;
}

uchar View::GetVisibilityPassIndex(int x, int y) const
{
	return m_visibility[GetIndexByLocal(x, y)];
}

void View::SetLocationLocal(int x, int y, Location location)
{
	m_locations[GetIndexByLocal(x, y)] = location;
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
	void Calculate(View& view, Location location, uchar maxPass)
	{
		view.ResetAt(location);

		CalculateQuadrant(view, West, maxPass);
		CalculateQuadrant(view, East, maxPass);
		CalculateQuadrant(view, North, maxPass);
		CalculateQuadrant(view, South, maxPass);

		//CalculateQuadrant(view, North, maxPass);
	}

	void CalculateQuadrant(View& view, Direction direction, uchar maxPass)
	{
		Row start = Row(1, 1, Fraction(-1, 1), Fraction(1, 1));
		Scan(view, direction, start, maxPass);
	}

	void Scan(View& view, Direction direction, Row& row, uchar maxPass)
	{
		if (row.m_depth > view.GetRadius()) { return; }
		if (row.m_pass > maxPass) { return; }

		int minCol = row.GetMinCol();
		int maxCol = row.GetMaxCol();

		Location prevTile = Location();

		for (int col = minCol; col <= maxCol; col++)
		{
			Vec2 pos = Transform(direction, col, row.m_depth);
			Location tile = GetTileByRowParent(view, direction, col, row);

			bool shouldOverwrite = ShouldOverwrite(view, pos.x, pos.y, row.m_pass);

			if ((IsWall(tile) || IsSymmetric(row, col)) && shouldOverwrite)
			{
				SetTile(view, direction, col, row.m_depth, tile);
				Reveal(view, direction, col, row.m_depth, row.m_pass);
			}
			if (BlocksVision(prevTile) && AllowsVision(tile))
			{
				row.m_startSlope = Slope(col, row.m_depth);
			}
			if (AllowsVision(prevTile) && BlocksVision(tile))
			{
				//Move to next row!
				Row nextRow = Row(row.m_pass, row.m_depth + 1, row.m_startSlope, Slope(col, row.m_depth));
				Scan(view, direction, nextRow, maxPass);
			}

			if (IsFloor(tile) && IsSymmetric(row, col) && RequiresRecast(tile) && shouldOverwrite)
			{
				//Scan the other side first
				if (col != maxCol)
				{
					Row nextRow = Row(row.m_pass, row.m_depth, OppositeSlope(col, row.m_depth), row.m_endSlope);
					Scan(view, direction, nextRow, maxPass);
				}

				//Scan next pass
				Reveal(view, direction, col, row.m_depth, row.m_pass + 1);
				Fraction min = std::max(row.m_startSlope, Slope(col, row.m_depth));
				Fraction max = std::min(row.m_endSlope, OppositeSlope(col, row.m_depth));

				Row recurseRow = Row(row.m_pass + 1, row.m_depth + 1, min, max);
				Scan(view, direction, recurseRow, maxPass);
				return;
			}

			prevTile = tile;
		}

		if (!BlocksVision(prevTile))
		{
			//Scan next row!
			Row nextRow = Row(row.m_pass, row.m_depth + 1, row.m_startSlope, row.m_endSlope);
			Scan(view, direction, nextRow, maxPass);
		}
	}

	void ResolveTile(View& view, Direction direction, int col, int row)
	{
		//Fraction slope = CenterSlope(col, row);

		int parentRow = row - 1;
		int parentCol = 0;

		int colA = FractionMultiplyRoundDown(parentRow, Slope(col, row));
		int colB = FractionMultiplyRoundUp(parentRow, OppositeSlope(col, row));

		Location tileA = GetTile(view, direction, colA, parentRow);
		Location tileB = GetTile(view, direction, colB, parentRow);

		if (colA == colB)
		{
			parentCol = colA;
		}
		else if (!tileA.GetValid() || !tileB.GetValid()) // If one of them isn't valid, we haven't shot any rays through it - we can skip!
		{
			parentCol = (tileA.GetValid()) ? colA : colB;
		}
		else if (IsWall(tileA)) //If one is a wall, the other must not be (since we can see through it) - pick the opposite
		{
			parentCol = colB;
		}
		else if (IsWall(tileB)) //Same as above
		{
			parentCol = colA;
		}
		else
		{
			//Vec2
			//Tiebreaker - we have two valid, open squares. Both are sending light into this square

			//Switch up our casting to using our center position - we're looking for closer alignments now
			int centerLow = FractionMultiplyRoundDown(parentRow, CenterSlope(col, row));
			int centerHigh = FractionMultiplyRoundUp(parentRow, CenterSlope(col, row));

			ASSERT(centerLow == colA || centerLow == colB);
			ASSERT(centerHigh == colA || centerHigh == colB);


			if (centerLow == centerHigh)
			{
				//Common case - one of them is more in line with this tile, so we should pick that one.
				parentCol = centerLow;
			}
			else
			{
				//Uncommon case - we're split perfectly down the middle. Tiebreak to the center.
				parentCol = (abs(colA) < abs(colB)) ? colA : colB;
			}
		}

		Vec2 parentLoc = Transform(direction, parentCol, parentRow);
		Vec2 childLoc = Transform(direction, col, row);

		Vec2 offset = childLoc - parentLoc;

		Location parent = GetTile(view, direction, parentCol, parentRow);
		ASSERT(parent.GetValid());

		SetTile(view, direction, col, row, GetTile(view, direction, parentCol, parentRow).Traverse(offset.x, offset.y));
	}

	Location GetTileByRowParent(View& view, Direction direction, int col, const Row& row)
	{
		Fraction minSlope = row.m_startSlope;//std::max(row.m_startSlope, Slope(col, row.m_depth));
		Fraction maxSlope = row.m_endSlope; //std::min(row.m_endSlope, OppositeSlope(col, row.m_depth));

		int parentRow = 0;
		int parentCol = 0;

		//Scan Phase - Search back to the most recent, visible, same pass parent
		bool found = false;
		int searchDepth = row.m_depth - 1;
		while (searchDepth >= 0)
		{
			int minCol = FractionMultiplyRoundUp(searchDepth, minSlope);
			int maxCol = FractionMultiplyRoundDown(searchDepth, maxSlope);

			for (int searchCol = minCol; searchCol <= maxCol; searchCol++)
			{
				Vec2 viewPos = Transform(direction, searchCol, searchDepth);
				if (view.GetVisibilityPassIndex(viewPos.x, viewPos.y) == row.m_pass)
				{
					parentCol = searchCol;
					parentRow = searchDepth;
					found = true;
					break;
				}
			}

			if (found)
			{
				break;
			}

			searchDepth--;
		}

		ASSERT(searchDepth != -1); //Implies we overshot the origin.
		ASSERT(found);

		Vec2 parentLoc = Transform(direction, parentCol, parentRow);
		Vec2 childLoc = Transform(direction, col, row.m_depth);

		Vec2 offset = childLoc - parentLoc;

		Location parent = GetTile(view, direction, parentCol, parentRow);

		Location child = BresenhamTraverse(parent, offset);

		return child;
	}

	//Determines if a new entry can overwrite an existing one.
	//Change this to control whether baselines passes are on top or below portal passes.
	bool ShouldOverwrite(const View& view, int col, int row, uchar pass)
	{
		uchar current = view.GetVisibilityPassIndex(col, row);
		return (current == 0) || (current >= pass);
	}

	Location BresenhamTraverse(Location start, Vec2 offset)
	{
		ASSERT(start.GetValid());
		Location traversal = start;
		int x0 = 0;
		int y0 = 0;
		int dx = abs(offset.x - 0), sx = 0 < offset.x ? 1 : -1;
		int dy = -abs(offset.y - 0), sy = 0 < offset.y ? 1 : -1;
		int err = dx + dy, e2; /* error value e_xy */

		for (;;) {  /* loop */
			Vec2 pos = Vec2(40, 20) + Vec2(x0, -y0);
			if (x0 == offset.x && y0 == offset.y)
			{
				return traversal;
			}
			e2 = 2 * err;
			int xOffset = 0;
			int yOffset = 0;

			if (e2 >= dy) { err += dy; x0 += sx; xOffset += sx; } /* e_xy+e_x > 0 */
			if (e2 <= dx) { err += dx; y0 += sy; yOffset += sy; } /* e_xy+e_y < 0 */

			traversal = traversal.Traverse(xOffset, yOffset);
			ASSERT(traversal.GetValid());
		}
	}

	Location GetTile(View& view, Direction direction, int col, int row)
	{
		Vec2 pos = Transform(direction, col, row);

		return view.GetLocationLocal(pos.x, pos.y);
	}

	void SetTile(View& view, Direction direction, int col, int row, Location location)
	{
		Vec2 pos = Transform(direction, col, row);

		view.SetLocationLocal(pos.x, pos.y, location);
	}

	Vec2 Transform(Direction direction, int col, int row)
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

		return Vec2(x, y);
	}

	Vec2 GetBresenhamParent(int x, int y)
	{
		bool negX = x < 0;
		bool nexY = y < 0;

		x = abs(x);
		y = abs(y);

		bool flipped = (x > y);
		if (flipped)
		{
			int hold = x;
			x = y;
			y = hold;
		}



		return Vec2(0, 0);
	}

	bool IsSymmetric(const Row& row, int col)
	{
		return IsSymmetric(col, row.m_depth, row.m_startSlope, row.m_endSlope);
	}

	bool IsSymmetric(const int col, const int row, const Fraction start, const Fraction end)
	{
		bool withinLower = (col * start.m_denominator >= row * start.m_numerator);
		bool withinUpper = (col * end.m_denominator <= row * end.m_numerator);
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

	bool RequiresRecast(Location location)
	{
		THandle<TileNeighbors> neighbors = location.GetNeighbors();
		return neighbors.IsValid();
	}

	bool BlocksVision(Location location)
	{
		return IsWall(location) || RequiresRecast(location);
	}

	bool AllowsVision(Location location)
	{
		return IsFloor(location) && !RequiresRecast(location);
	}

	void Reveal(View& view, Direction direction, int col, int row, uchar pass)
	{
		Vec2 pos = Transform(direction, col, row);

		view.Mark(pos.x, pos.y, pass);
	}

	Fraction Slope(int col, int row)
	{
		return Fraction(2 * col - 1, 2 * row);
	}

	Fraction CenterSlope(int col, int row)
	{
		return Fraction(col, row);
	}

	Fraction OppositeSlope(int col, int row)
	{
		return Fraction(2 * col + 1, 2 * row);
	}
}