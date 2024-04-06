#pragma once
#include "../Map/Map.h"

/*
namespace LOS
{
	class View
	{
	public:
		View();

		void SetRadius(int radius);
		void SetCenter(Location location);

		void Clear();
		void Mark(Location location, bool value);
	};

	struct Row
	{
		Location* locations;
		int numLocations;
		int startsAt;
		Direction walkDir;
		int startSlopeNumerator;
		int startSlopeDenominator;
		int endSlopeNumerator;
		int endSlopeDenominator;
		int currentDepth;
	};

	static bool IsWall(Location location)
	{
		if (!location.GetValid())
		{
			return false;
		}
		return location->m_backingTile->m_blocksVision;
	}

	static bool IsFloor(Location location)
	{
		if (!location.GetValid())
		{
			return false;
		}
		return !location->m_backingTile->m_blocksVision;
	}

	static bool IsSymmetric(Row& row, int index)
	{
		int column = row.startsAt + index;

		//Overly complicated math keeps things in int space - perfect accuracy!
		bool isInLower = (column * row.startSlopeDenominator) >= (row.currentDepth * row.startSlopeNumerator);
		bool isInUpper = (column * row.endSlopeDenominator) <= (row.currentDepth * row.endSlopeNumerator);
		return isInLower && isInUpper;
	}

	void Reveal(View& view, Row& row, int index)
	{

	}

	void MatchStartSlope(Row& row, int index)
	{

	}

	void SetupNextRow(Row& oldRow, Row& newRow)
	{

	}

	static void ScanRow(View& view, int radius, Row& row)
	{
		Location previousTile;
		ASSERT(!previousTile.GetValid());
		for (int i = 0; i < row.numLocations; i++)
		{
			Location tile = row.locations[i];
			if (IsWall(tile) || IsSymmetric(row, i))
			{
				Reveal(view, row, i);
			}
			if (IsWall(previousTile) && IsFloor(tile))
			{
				MatchStartSlope(row, index);
			}
			if (IsFloor(previousTile) && IsWall(tile))
			{
				//Generate next row
				Row nextRow;
				nextRow.locations = alloca(row.numLocations + 2);
				SetupNextRow(oldRow, newRow);

				//Clip ending to match!
			}

			previousTile = tile;
		}

		if (IsFloor(previousTile))
		{
			//Recurse!
		}
	}

	static void CalculateQuadrant(View& view, Location location, int radius, Direction direction)
	{

	}

	static void CalculateLOSAt(View& view, Location location, int radius)
	{
		view.SetRadius(radius);
		view.Clear();
		view.SetCenter(location);

		// Mark center as seen
		view.Mark(location, true);

		CalculateQuadrant(view, location, radius, Direction::East);
		CalculateQuadrant(view, location, radius, Direction::West);
		CalculateQuadrant(view, location, radius, Direction::North);
		CalculateQuadrant(view, location, radius, Direction::South);
	}
}*/