#pragma once
#include "../Core/CoreDataTypes.h"
#include <vector>

/*
	Non-euclidean Recursive Symmetric Shadowcasting

	Based on Albert Ford's orginal algorithm

	Modified to work in non-euclidean spaces. The main changes are that space-breaking
	moves are allowed to create new rows, and that new rows resolve their world-space
	position by using linear interpolation to choose a resolved parent and iterate from
	them.
*/

class BackingTile;
class TileStats;
class TileNeighbors;
class Tile;
class Location;

using namespace std;

#ifdef _DEBUG
#define DEBUG_HOTSPOTS
#endif

class View
{
public:
	View() {}

	void SetRadius(int radius);
	void SetRadiusOnlyUpsize(int radius);
	int GetRadius() { return m_radius; }
	void ResetAt(Location location);

	int GetIndexByLocal(int x, int y) const;

	void Mark(int x, int y, uchar value);

	//Getters
	Location GetLocationLocal(int x, int y) const;
	bool GetVisibilityLocal(int x, int y) const;
	uchar GetVisibilityPassIndex(int x, int y) const;

	//Setters
	void SetLocationLocal(int x, int y, Location location);

	//Debug tools (should compile out)
	void Debug_AddHeatLocal(int x, int y);
	int Debug_GetHeatLocal(int x, int y);
	int Debug_GetMaxHeat();
	int Debug_GetSumHeat();
	int Debug_GetNumRevealed();
	float Debug_GetHeatPercentageLocal(int x, int y);

protected:
	int m_radius = -1;
	vector<Location> m_locations;
	vector<uchar> m_visibility;
#ifdef DEBUG_HOTSPOTS
public:
	vector<int> m_heat;
	int m_maxHeat = 0;
	int m_sumHeat = 0;
	int m_numRevealed = 0;
#endif
};

namespace LOS
{
	struct Fraction
	{
		int m_numerator;
		int m_denominator;

		Fraction(int numerator, int denominator) : m_numerator(numerator), m_denominator(denominator) {}
	};

	inline int FractionMultiplyRoundUp(int value, Fraction fraction)
	{
		int numerator = 2 * value * fraction.m_numerator + fraction.m_denominator;
		int denominator = 2 * fraction.m_denominator;

		return IntDivisionFloor(numerator, denominator);
	}

	inline int FractionMultiplyRoundDown(int value, Fraction fraction)
	{
		int numerator = 2 * value * fraction.m_numerator - fraction.m_denominator;
		int denominator = 2 * fraction.m_denominator;

		return IntDivisionCeil(numerator, denominator);
	}

	inline bool operator <(Fraction lhs, Fraction rhs)
	{
		return (lhs.m_numerator * rhs.m_denominator) < (lhs.m_denominator * rhs.m_numerator);
	}

	inline bool operator >(Fraction lhs, Fraction rhs)
	{
		return (lhs.m_numerator * rhs.m_denominator) > (lhs.m_denominator * rhs.m_numerator);
	}

	struct Row
	{
		uchar m_pass;
		int m_depth;
		Fraction m_startSlope;
		Fraction m_endSlope;
		Direction m_rotation;

		Row(uchar pass, int depth, Fraction startSlope, Fraction endSlope, Direction rotation) :
			m_pass(pass),
			m_depth(depth),
			m_startSlope(startSlope),
			m_endSlope(endSlope),
			m_rotation(rotation){}

		int GetMinCol()
		{
			return FractionMultiplyRoundUp(m_depth, m_startSlope);
		}

		int GetMaxCol()
		{
			return FractionMultiplyRoundDown(m_depth, m_endSlope);
		}
	};

	//Core shadowcasting
	void Calculate(View& view, Location location, Direction rotation, uchar maxPass = 255);
	void CalculateQuadrant(View& view, View& scratch, Direction direction, Direction rotation, uchar maxPass);
	void Scan(View& view, View& scratch, Direction direction, Row& row, uchar maxPass);

	//Recursive mapping
	Location GetTileByRowParent(View& view, View& scratch, Direction direction, Direction rotation, int col, const Row& row);
	bool ShouldOverwrite(const View& view, int col, int row, uchar pass);

	Location GetTile(View& view, Direction direction, int col, int row);
	void SetTile(View& view, Direction direction, int col, int row, Location location);

	//Utility functions
	Vec2 Transform(Direction direction, int col, int row);
	bool IsSymmetric(const Row& row, int col);
	bool IsSymmetric(const int col, const int row, const Fraction start, const Fraction end);
	bool IsWall(Location location);
	bool IsFloor(Location location);
	bool RequiresRecast(Location location);
	bool BlocksVision(Location location);
	bool AllowsVision(Location location);
	void Reveal(View& view, Direction direction, int col, int row, uchar pass);
	Fraction Slope(int col, int row);
	Fraction CenterSlope(int col, int row);
	Fraction OppositeSlope(int col, int row);
}