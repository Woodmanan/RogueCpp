#include "MapUtils.h"
#include "Core/Collections/StackArray.h"
#include "Core/CoreDataTypes.h"
#include "Core/Materials/Materials.h"

namespace MapUtils
{
	void CreatePortal(Location open, Direction openDir, Location exit, Direction exitDir)
	{
		//Set up the defaults first
		open.CreateDefaultNeighbors();		
		exit.CreateDefaultNeighbors();		

		Direction rotation = FindRotationBetween(openDir, exitDir);

		Location exitLoc = exit.GetNeighbor(exitDir);
		Location exitLocCL = exit.GetNeighbor(TurnClockwise(exitDir));
		Location exitLocCCL = exit.GetNeighbor(TurnCounterClockwise(exitDir));

		Direction reverseOpen = Reverse(openDir);
		Direction reverseExit = Reverse(exitDir);
		Location openLoc = open.GetNeighbor(reverseOpen);
		Location openLocCL = open.GetNeighbor(TurnClockwise(reverseOpen));
		Location openLocCCL = open.GetNeighbor(TurnCounterClockwise(reverseOpen));

		open.SetNeighbor(openDir, exitLoc, rotation);
		open.SetNeighbor(TurnClockwise(openDir), exitLocCL, rotation);
		open.SetNeighbor(TurnCounterClockwise(openDir), exitLocCCL, rotation);

		exit.SetNeighbor(reverseExit, openLoc, ReverseRotation(rotation));
		exit.SetNeighbor(TurnClockwise(reverseExit), openLocCL, ReverseRotation(rotation));
		exit.SetNeighbor(TurnCounterClockwise(reverseExit), openLocCCL, ReverseRotation(rotation));		
	}


	void BreakWall(Location location)
	{
		ASSERT(location->m_wall);

		THandle<TileStats> stats = location.GetOrCreateInstanceData();

		STACKARRAY(Material, materialsToMove, 20);
		for (const Material& material : stats->m_volumeMaterials.m_materials)
		{
			MaterialDefinition definition = GetMaterialManager()->GetMaterialDefinition(material);
			if (definition.phase == Phase::Solid)
			{
				materialsToMove.push_back(material);
			}
		}

		for (const Material& material : materialsToMove)
		{
			stats->m_volumeMaterials.RemoveMaterial(material);
			stats->m_floorMaterials.AddMaterial(material);
		}

		location->m_wall = false;
		location->m_dirty = true;
	}
}
