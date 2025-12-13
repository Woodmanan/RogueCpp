#include "LOS.h"
#include "Map/Map.h"
#include "Debug/Profiling.h"
#include "Core/Monster/Monster.h"
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
		m_rotations.resize(numTiles);
		
#ifdef DEBUG_HOTSPOTS
		m_heat.resize(numTiles);
#endif
	}
}

void View::SetRadiusOnlyUpsize(int radius)
{
	int diameter = (2 * radius) + 1;
	int numTiles = diameter * diameter;

	if (m_locations.size() < numTiles)
	{
		m_locations.resize(numTiles);
		m_visibility.resize(numTiles);
		m_rotations.resize(numTiles);
#ifdef DEBUG_HOTSPOTS
		m_heat.resize(numTiles);
#endif
	}

	m_radius = radius;
}

int View::GetIndexByLocal(int x, int y) const
{
	ROGUE_PROFILE_SECTION("View::GetIndexByLocal");
	ASSERT(x >= -m_radius && x <= m_radius);
	ASSERT(y >= -m_radius && y <= m_radius);

	return (x + m_radius) + (y + m_radius) * (2 * m_radius + 1);
}

void View::ResetAt(Location location)
{
	int diameter = (2 * m_radius) + 1;
	int numTiles = diameter * diameter;

	std::fill(m_locations.begin(), m_locations.begin() + numTiles, Location());
	std::fill(m_visibility.begin(), m_visibility.begin() + numTiles, 0);
	std::fill(m_rotations.begin(), m_rotations.begin() + numTiles, North);

	m_locations[GetIndexByLocal(0, 0)] = location;
	m_visibility[GetIndexByLocal(0, 0)] = 1;

#ifdef DEBUG_HOTSPOTS
	std::fill(m_heat.begin(), m_heat.begin() + numTiles, 0);
	m_maxHeat = 0;
	m_sumHeat = 0;
	m_numRevealed = 0;
#endif
}

void View::Mark(int x, int y, uchar value)
{
	m_visibility[GetIndexByLocal(x, y)] = value;
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
	ROGUE_PROFILE_SECTION("View::GetVisibilityPassIndex");
	return m_visibility[GetIndexByLocal(x, y)];
}

Direction View::GetRotationLocal(int x, int y) const
{
	return m_rotations[GetIndexByLocal(x, y)];
}

void View::SetLocationLocal(int x, int y, Location location)
{
	m_locations[GetIndexByLocal(x, y)] = location;
}

void View::SetRotationLocal(int x, int y, Direction direction)
{
	m_rotations[GetIndexByLocal(x, y)] = direction;
}

void View::SetVisibilityLocal(int x, int y, bool visible)
{
	m_visibility[GetIndexByLocal(x, y)] = visible;
}

void View::Debug_AddHeatLocal(int x, int y)
{
#ifdef DEBUG_HOTSPOTS
	int index = GetIndexByLocal(x, y);
	m_heat[index]++;
	m_maxHeat = std::max(m_heat[index], m_maxHeat);
	m_sumHeat++;
#endif
}

int View::Debug_GetHeatLocal(int x, int y)
{
#ifdef DEBUG_HOTSPOTS
	int index = GetIndexByLocal(x, y);
	return m_heat[index];
#else
	return 0;
#endif
}

int View::Debug_GetMaxHeat()
{
#ifdef DEBUG_HOTSPOTS
	return m_maxHeat;
#else
	return 0;
#endif
}

int View::Debug_GetSumHeat()
{
#ifdef DEBUG_HOTSPOTS
	return m_sumHeat;
#else
	return 0;
#endif
}

int View::Debug_GetNumRevealed()
{
#ifdef DEBUG_HOTSPOTS
	return m_numRevealed;
#else
	return 0;
#endif
}

float View::Debug_GetHeatPercentageLocal(int x, int y)
{
#ifdef DEBUG_HOTSPOTS
	int index = GetIndexByLocal(x, y);
	return ((float)m_heat[index]) / max(m_maxHeat, 1);
#else
	return 0.0f;
#endif
}

namespace LOS
{
	void Calculate(THandle<Monster> monster, uchar maxPass)
	{
		ASSERT(monster.IsValid());
		Calculate(monster->GetView(), monster->GetLocation(), monster->GetRotation(), maxPass);
	}

	void Calculate(View& view, Location location, Direction rotation, uchar maxPass)
	{
		ROGUE_PROFILE_SECTION("LOS::Calculate");
		//Set scratch to match, iff it's smaller than needed
		static View scratch;
		scratch.SetRadiusOnlyUpsize(view.GetRadius());

		//Reset the views
		view.ResetAt(location);
		scratch.ResetAt(location);

		//Iterate each quadrant once
		CalculateQuadrant(view, scratch, West,  rotation, maxPass);
		CalculateQuadrant(view, scratch, East,  rotation, maxPass);
		CalculateQuadrant(view, scratch, North, rotation, maxPass);
		CalculateQuadrant(view, scratch, South, rotation, maxPass);
	}

	void CalculateQuadrant(View& view, View& scratch, Direction direction, Direction rotation, uchar maxPass)
	{
		ROGUE_PROFILE_SECTION("LOS::CalculateQuadrant");
		Row start = Row(1, 1, Fraction(-1, 1), Fraction(1, 1));
		scratch.SetRotationLocal(0, 0, rotation);
		Scan(view, scratch, direction, start, maxPass);
	}

	void Scan(View& view, View& scratch, Direction direction, Row& row, uchar maxPass)
	{
		ROGUE_PROFILE_SECTION("LOS::Scan");
		if (row.m_depth > view.GetRadius()) { return; }
		if (row.m_pass > maxPass) { return; }

		int minCol = row.GetMinCol();
		int maxCol = row.GetMaxCol();

		Location prevTile = Location();

		for (int col = minCol; col <= maxCol; col++)
		{
			Vec2 pos = Transform(direction, col, row.m_depth);
			auto move = GetTileByRowParent(view, scratch, direction, col, row);
			view.Debug_AddHeatLocal(pos.x, pos.y);

			Location tile = move.first;
			Direction rotation = move.second;

			if ((IsWall(tile) || IsSymmetric(row, col)))
			{
				if (ShouldOverwrite(view, pos.x, pos.y, row.m_pass))
				{
					SetTile(view, direction, col, row.m_depth, tile);
					SetRotation(view, direction, col, row.m_depth, rotation);
					Reveal(view, direction, col, row.m_depth, row.m_pass);
				}
			}
			if (BlocksVision(prevTile) && AllowsVision(tile))
			{
				row.m_startSlope = Slope(col, row.m_depth);
			}
			if (AllowsVision(prevTile) && BlocksVision(tile))
			{
				//Move to next row!
				Row nextRow = Row(row.m_pass, row.m_depth + 1, row.m_startSlope, Slope(col, row.m_depth));
				Scan(view, scratch, direction, nextRow, maxPass);
			}
			if (IsFloor(tile) && RequiresRecast(tile))
			{
				//This is a portal - scan recursive pass through it's sightlines
				Fraction min = std::max(row.m_startSlope, Slope(col, row.m_depth));
				Fraction max = std::min(row.m_endSlope, OppositeSlope(col, row.m_depth));
				Row recurseRow = Row(row.m_pass + 1, row.m_depth + 1, min, max);
				Scan(view, scratch, direction, recurseRow, maxPass);
			}

			prevTile = tile;
		}

		if (!BlocksVision(prevTile))
		{
			//Scan next row!
			Row nextRow = Row(row.m_pass, row.m_depth + 1, row.m_startSlope, row.m_endSlope);
			Scan(view, scratch, direction, nextRow, maxPass);
		}
	}

	std::pair<Location, Direction> GetTileByRowParent(View& view, View& scratch, Direction direction, int col, const Row& row)
	{
		ROGUE_PROFILE_SECTION("LOS::GetTileByRowParent");
		Fraction minSlope = row.m_startSlope;
		Fraction maxSlope = row.m_endSlope;

		int parentRow = row.m_depth - 1;

		//Scan Phase - Search back to the most recent, visible, same pass parent
		int minCol = FractionMultiplyRoundUp(parentRow, minSlope);
		int maxCol = FractionMultiplyRoundDown(parentRow, maxSlope);

		//Bounding box snap for location
		int parentCol = std::min(std::max(minCol, col), maxCol);

		Vec2 parentLoc = Transform(direction, parentCol, parentRow);
		Vec2 childLoc = Transform(direction, col, row.m_depth);

		Vec2 offset = childLoc - parentLoc;

		ASSERT(std::abs(offset.x) <= 1 && std::abs(offset.y) <= 1);

		Location parent = GetTile(scratch, direction, parentCol, parentRow);
		Direction parentDir = GetRotation(scratch, direction, parentCol, parentRow);

		auto childMove = parent.Traverse(offset, parentDir);

		//No matter what, write this tile into the scratch pad. Our recursions will either ignore it spatially or need it set.
		SetTile(scratch, direction, col, row.m_depth, childMove.first);
		Direction newRotation = Rotate(parentDir, childMove.second);

		SetRotation(scratch, direction, col, row.m_depth, newRotation);

		return std::make_pair(childMove.first, newRotation);
	}

	//Determines if a new entry can overwrite an existing one.
	//Switching this logic allows blocking walls to show up, but removes some of the sightlines guarantees
	bool ShouldOverwrite(const View& view, int col, int row, uchar pass)
	{
		uchar current = view.GetVisibilityPassIndex(col, row);
		return (current == 0) || (current >= pass);
	}

	Location GetTile(View& view, Direction direction, int col, int row)
	{
		ROGUE_PROFILE_SECTION("LOS::GetTile");
		Vec2 pos = Transform(direction, col, row);

		return view.GetLocationLocal(pos.x, pos.y);
	}

	void SetTile(View& view, Direction direction, int col, int row, Location location)
	{
		ROGUE_PROFILE_SECTION("LOS::SetTile");
		Vec2 pos = Transform(direction, col, row);

		view.SetLocationLocal(pos.x, pos.y, location);
	}

	Direction GetRotation(View& view, Direction direction, int col, int row)
	{
		ROGUE_PROFILE_SECTION("LOS::GetRotation");
		Vec2 pos = Transform(direction, col, row);

		return view.GetRotationLocal(pos.x, pos.y);
	}

	void SetRotation(View& view, Direction direction, int col, int row, Direction rotation)
	{
		ROGUE_PROFILE_SECTION("LOS::SetRotation");
		Vec2 pos = Transform(direction, col, row);

		view.SetRotationLocal(pos.x, pos.y, rotation);
	}

	Vec2 Transform(Direction direction, int col, int row)
	{
		ROGUE_PROFILE_SECTION("LOS::Transform");
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
		default:
			break;
		}

		return Vec2(x, y);
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
		return location.GetValid() && location->m_wall;
	}

	bool IsFloor(Location location)
	{
		return location.GetValid() && !location->m_wall;
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
		ROGUE_PROFILE_SECTION("LOS::Reveal");
		Vec2 pos = Transform(direction, col, row);

#ifdef DEBUG_HOTSPOTS
		if (!view.GetVisibilityLocal(pos.x, pos.y))
		{
			view.m_numRevealed++;
		}
#endif

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