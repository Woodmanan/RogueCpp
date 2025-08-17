#include "PlayerData.h"
#include "Data/Serialization/Serialization.h"
#include "Game/Game.h"

PlayerData::PlayerData()
{
	BackingTile emptyTile;
	emptyTile.m_renderCharacter = ' ';
	emptyTile.m_foregroundColor = Color(0, 0, 0);
	emptyTile.m_backgroundColor = Color(0, 0, 0);
	m_backingTiles[THandle<BackingTile>()] = emptyTile;
}

void PlayerData::UpdateViewGame(View& newView)
{
	ROGUE_PROFILE_SECTION("Update view: Game side");
	PackedStream afterStream;

	int maxRadius = newView.GetRadius();
	bool updatedBacking = false;
	bool updatedTiles = false;

	for (int i = -maxRadius; i <= maxRadius; i++)
	{
		for (int j = -maxRadius; j <= maxRadius; j++)
		{
			bool visible = newView.GetVisibilityLocal(i, j);
			Location loc = newView.GetLocationLocal(i, j);
			if (visible && loc.GetValid())
			{
				THandle<BackingTile> tile = newView.GetLocationLocal(i, j)->m_backingTile;
				if (!m_backingTiles.contains(tile))
				{
					updatedBacking = true;
					m_backingTiles[tile] = tile.GetReference();
				}
			}
		}
	}

	m_memory.Update(newView);

	//TODO: Make this work by delta!
	{
		ROGUE_PROFILE_SECTION("Serialization");
		Serialization::Write(afterStream, "View", newView);

		Serialization::Write(afterStream, "Memory", m_memory);

		Serialization::Write(afterStream, "Update Backing", updatedBacking);
		if (updatedBacking)
		{
			Serialization::Write(afterStream, "Backing Tiles", m_backingTiles);
		}
	}

	afterStream.AllWritesFinished();

	m_currentView = newView;
	Game::game->CreateOutput<ViewUpdated>(afterStream);
}

void PlayerData::UpdateViewPlayer(std::shared_ptr<TOutput<ViewUpdated>> updated)
{
	PackedStream stream;

	std::shared_ptr<VectorBackend> backend = dynamic_pointer_cast<VectorBackend>(stream.GetDataBackend());
	ASSERT(backend != nullptr);
	backend->m_data.insert(backend->m_data.end(), updated->m_data.begin(), updated->m_data.end());

	Serialization::Read(stream, "View", m_currentView);

	Serialization::Read(stream, "Memory", m_memory);
	
	if (Serialization::Read<PackedStream, bool>(stream, "Update Backing"))
	{
		Serialization::Read(stream, "Backing Tiles", m_backingTiles);
	}
}

BackingTile& PlayerData::GetTileForLocal(int x, int y)
{
	Tile& tile = m_memory.GetTileByLocal(x, y);
	return m_backingTiles[tile.m_backingTile];
}