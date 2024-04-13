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

protected:
	int m_radius = -1;
	vector<Location> m_locations;
	vector<bool> m_visibility;
};

int IntLerp(int a, int b, int numerator, int denominator);

namespace LOS
{
	void Calculate(View& view, Location location);
}