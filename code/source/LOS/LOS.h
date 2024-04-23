#pragma once
#include "../Map/Map.h"

/*
	Non-euclidean Recursive Symmetric Shadowcasting

	Based on Albert Ford's orginal algorithm

	Modified to work in non-euclidean spaces. The main changes are that space-breaking
	moves are allowed to create new rows, and that new rows resolve their world-space
	position by using linear interpolation to choose a resolved parent and iterate from
	them.
*/

class View
{
public:
	View() {}

	void SetRadius(int radius);
	int GetRadius() { return m_radius; }
	void ResetAt(Location location);

	int GetIndexByLocal(int x, int y);

	void Clear();
	void Mark(int x, int y, bool value);

	//Local Space Calculations
	Vec2 GetSensibleParent(int x, int y);
	void BuildLocalSpace();
	void BuildLocalSpaceTile(int x, int y);


	//Getters
	Location GetLocationLocal(int x, int y);
	bool GetVisibilityLocal(int x, int y);

	//Setters
	void SetLocationLocal(int x, int y, Location location);

protected:
	int m_radius = -1;
	vector<Location> m_locations;
	vector<bool> m_visibility;
};

int IntLerp(int a, int b, int numerator, int denominator);

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

	struct Row
	{
		int m_depth;
		Fraction m_startSlope;
		Fraction m_endSlope;

		Row(int depth, Fraction startSlope, Fraction endSlope) :
			m_depth(depth),
			m_startSlope(startSlope),
			m_endSlope(endSlope) {}

		int GetMinCol()
		{
			return FractionMultiplyRoundUp(m_depth, m_startSlope);
		}

		int GetMaxCol()
		{
			return FractionMultiplyRoundDown(m_depth, m_endSlope);
		}
	};

	void Calculate(View& view, Location location);

	void CalculateQuadrant(View& view, Direction direction);

	void Scan(View& view, Direction direction, Row& row);

	void ResolveTile(View& view, Direction direction, int col, int row);
	Location GetTile(View& view, Direction direction, int col, int row);
	void SetTile(View& view, Direction direction, int col, int row, Location location);

	Vec2 Transform(Direction direction, int col, int row);

	void ResolveTileBresenham(View& view, Direction direction, int col, int row);

	bool IsSymmetric(Row& row, int col);
	bool IsWall(Location location);
	bool IsFloor(Location location);
	bool RequiresRecast(Location location);
	void Reveal(View& view, Direction direction, int col, int row);
	Fraction Slope(int col, int row);
	Fraction CenterSlope(int col, int row);
	Fraction OppositeSlope(int col, int row);
}