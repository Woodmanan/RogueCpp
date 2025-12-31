#pragma once
#include <vector>
#include "Core/CoreDataTypes.h"
#include "Core/Collections/StackArray.h"
#include "Core/Monster/Monster.h"

namespace Pathfinding
{
	using std::vector;

	static constexpr float SQRT_TWO = 1.4142135623f;
	static constexpr float EPSILON = 0.001f; //Acceptable difference for cleaning up the path

	using MovementCallback = std::function<void(Location, MovementArray&)>;

	struct PathfindingSettings
	{
		int m_maxDepth = 1000;
		float m_maxCost = 1000.0f;
		Vec3 m_lowerBound = Vec3(0, 0, 0);
		Vec3 m_upperBound = Vec3(INT32_MAX, INT32_MAX, INT32_MAX);
		MovementCallback positionGenerator;
	};

	vector<Location> GetPath(Location from, Location to, const PathfindingSettings& settings);

	bool GetPath(Location source, Location destination, const PathfindingSettings& settings, StackArray<Location>& locations);
}