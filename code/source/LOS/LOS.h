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
	void SetRadiusOnlyUpsize(int radius);
	int GetRadius() { return m_radius; }
	void ResetAt(Location location);

	int GetIndexByLocal(int x, int y) const;

	void Clear();
	void Mark(int x, int y, uchar value);

	//Local Space Calculations
	Vec2 GetSensibleParent(int x, int y);
	void BuildLocalSpace();
	void BuildLocalSpaceTile(int x, int y);


	//Getters
	Location GetLocationLocal(int x, int y) const;
	bool GetVisibilityLocal(int x, int y) const;
	uchar GetVisibilityPassIndex(int x, int y) const;

	//Setters
	void SetLocationLocal(int x, int y, Location location);

protected:
	int m_radius = -1;
	vector<Location> m_locations;
	vector<uchar> m_visibility;
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
		int m_depth;
		Fraction m_startSlope;
		Fraction m_endSlope;
		uchar m_pass;

		Row(uchar pass, int depth, Fraction startSlope, Fraction endSlope) :
			m_pass(pass),
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

	void Calculate(View& view, Location location, uchar maxPass = 255);

	void CalculateQuadrant(View& view, View& scratch, Direction direction, uchar maxPass);

	void Scan(View& view, View& scratch, Direction direction, Row& row, uchar maxPass);

	void ResolveTile(View& view, Direction direction, int col, int row);
	Location GetTileByRowParent(View& view, View& scratch, Direction direction, int col, const Row& row);

	bool ShouldOverwrite(const View& view, int col, int row, uchar pass);
	Location BresenhamTraverse(Location start, Vec2 offset);

	Location GetTile(View& view, Direction direction, int col, int row);
	void SetTile(View& view, Direction direction, int col, int row, Location location);

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