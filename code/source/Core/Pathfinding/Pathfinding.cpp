#include "Pathfinding.h"
#include "Core/Monster/Monster.h"
#include "Map/Map.h"
#include <algorithm>
#include <unordered_map>

namespace Pathfinding
{
	//Static search data!
	struct SearchNode
	{
		SearchNode() : m_location(Location()), m_priority(FLT_MAX) {}

		SearchNode(Location location, float priority) : m_location(location), m_priority(priority) {}
		Location m_location;
		float m_priority;

		friend bool operator<(const SearchNode& lhs, const SearchNode& rhs);
	};

	bool operator<(const SearchNode& lhs, const SearchNode& rhs)
	{
		return lhs.m_priority > rhs.m_priority;
	}

	std::vector<SearchNode> frontier = std::vector<SearchNode>(400);
	std::unordered_map<Location, Location> parents;
	std::unordered_map<Location, float> costSoFar;

	bool CanAdd(Location source, Location tile, float cost, const PathfindingSettings& settings)
	{
		return (cost < settings.m_maxCost);
	}

	float GetNudgeValue(Location source, Location parent, Location tile)
	{
		Vec2 offset = parent.AsVec2() - tile.AsVec2();
		if (offset.x * offset.y != 0)
		{
			return EPSILON;
		}
		
		return 0;
	}

	float GetHeuristic(Location source, Location destination, Location parent, Location tile)
	{
		Vec2 offset = destination.AsVec2() - tile.AsVec2();
		return std::max(std::abs(offset.x), std::abs(offset.y));
	}

	vector<Location> GetPath(Location from, Location to, const PathfindingSettings& settings)
	{
		return vector<Location>();
	}

	bool GetPath(Location source, Location destination, const PathfindingSettings& settings, StackArray<Location>& locations)
	{
		ROGUE_PROFILE_SECTION("Pathfinding::GetPath");
		ASSERT(locations.empty());
		ASSERT(settings.positionGenerator);

		DEBUG_PRINT("SEARCHING [%d, %d] TO [%d, %d]", source.x(), source.y(), destination.x(), destination.y());

		frontier.clear();
		parents.clear();
		costSoFar.clear();

		parents[source] = Location();
		costSoFar[source] = 0.0f;
		
		frontier.push_back(SearchNode(source, 0.0f));
		std::push_heap(frontier.begin(), frontier.end(), std::less());
		ASSERT(std::is_heap(frontier.begin(), frontier.end(), std::less()));

		MovementArray next;

		while (!frontier.empty())
		{
			{
				ROGUE_PROFILE_SECTION("pop_heap");
				std::pop_heap(frontier.begin(), frontier.end(), std::less());
			}
			
			Location current = frontier.back().m_location;
			float priority = frontier.back().m_priority;
			frontier.pop_back();
			ASSERT(std::is_heap(frontier.begin(), frontier.end(), std::less()));

			DEBUG_PRINT("[%d, %d] : %f", current.x(), current.y(), priority);

			if (current == destination)
			{
				break;
			}

			{
				ROGUE_PROFILE_SECTION("FillValidMovements");
				next.clear();
				if (settings.positionGenerator)
				{
					settings.positionGenerator(current, next);
				}
			}

			for (Movement& movement : next)
			{
				ASSERT(movement.m_location.GetValid());
				float newCost = costSoFar[current] + movement.m_cost + GetNudgeValue(source, current, movement.m_location);

				if ((!costSoFar.contains(movement.m_location)) || (newCost < costSoFar[movement.m_location]))
				{
					if (!CanAdd(source, movement.m_location, newCost, settings))
					{
						continue;
					}

					costSoFar[movement.m_location] = newCost;
					float priority = newCost + GetHeuristic(source, destination, current, movement.m_location);

					DEBUG_PRINT("\t[%d, %d] : %f - %f", movement.m_location.x(), movement.m_location.y(), newCost, priority);

					frontier.push_back(SearchNode(movement.m_location, priority));
					{
						ROGUE_PROFILE_SECTION("push_heap");
						std::push_heap(frontier.begin(), frontier.end(), std::less());
					}				
					ASSERT(std::is_heap(frontier.begin(), frontier.end(), std::less()));

					parents[movement.m_location] = current;
				}
			}
		}

		if (parents.contains(destination))
		{
			Location current = destination;
			while (current.GetValid())
			{
				locations.push_back(current);
				current = parents[current];
			}

			locations.reverse();
			return true;
		}
		else
		{
			return false;
		}
	}
}