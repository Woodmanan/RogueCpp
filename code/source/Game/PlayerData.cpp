#include "PlayerData.h"
#include "Data/Serialization/Serialization.h"
#include "Game/Game.h"

void PlayerData::UpdateViewGame(View& newView)
{
	ROGUE_PROFILE_SECTION("Update view: Game side");
	DefaultStream afterStream;

	int maxRadius = newView.GetRadius();

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
					m_backingTiles[tile] = tile.GetReference();
				}

				if (!m_tileData.contains(loc))
				{
					m_tileData[loc] = tile;
				}
			}
		}
	}

	//TODO: Make this work by delta!
	Serialization::Write(afterStream, "View", newView);
	Serialization::Write(afterStream, "Backing Tiles", m_backingTiles);
	Serialization::Write(afterStream, "Tile Data", m_tileData);

	m_currentView = newView;
	Game::game->CreateOutput<TOutput<ViewUpdated>>(afterStream);
}

void PlayerData::UpdateViewPlayer(std::shared_ptr<TOutput<ViewUpdated>> updated)
{
	DefaultStream stream;

	std::shared_ptr<VectorBackend> backend = dynamic_pointer_cast<VectorBackend>(stream.GetDataBackend());
	ASSERT(backend != nullptr);
	backend->m_data.insert(backend->m_data.end(), updated->m_data.begin(), updated->m_data.end());

	Serialization::Read(stream, "View", m_currentView);
	Serialization::Read(stream, "Backing Tiles", m_backingTiles);
	Serialization::Read(stream, "Tile Data", m_tileData);
}

BackingTile& PlayerData::GetTileFor(Location location)
{
	return m_backingTiles[m_tileData[location]];
}