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

	Serialization::Write(afterStream, "First Send", !hasSent);
	if (!hasSent)
	{
		//Send all data first!
		Serialization::Write(afterStream, "View", newView);
		Serialization::Write(afterStream, "Memory", m_memory);
		Serialization::Write(afterStream, "Backing Tiles", m_backingTiles);
		hasSent = true;
	}

	Serialization::Write(afterStream, "MaxRadius", maxRadius);
	Serialization::Write(afterStream, "Position", m_memory.m_localPosition);
	for (int i = -maxRadius; i <= maxRadius; i++)
	{
		for (int j = -maxRadius; j <= maxRadius; j++)
		{
			bool visible = newView.GetVisibilityLocal(i, j);
			Serialization::Write(afterStream, "Visible", visible);

			if (visible)
			{
				Location loc = newView.GetLocationLocal(i, j);
				bool tileUpdate = (loc.GetValid() && !m_memory.ValidTile(i, j)) || (loc.GetTile() != m_memory.GetTileByLocal(i, j));
				Serialization::Write(afterStream, "Update Tile", tileUpdate);
				if (tileUpdate)
				{
					WriteTileUpdate(afterStream, i, j, loc.GetTile());
				}
			}
		}
	}

	m_memory.Update(newView);

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

	if (Serialization::Read<PackedStream, bool>(stream, "First Send"))
	{
		//Send all data first!
		Serialization::Read(stream, "View", m_currentView);
		Serialization::Read(stream, "Memory", m_memory);
		Serialization::Read(stream, "Backing Tiles", m_backingTiles);
	}

	int newRadius;
	Serialization::Read(stream, "MaxRadius", newRadius);
	Serialization::Read(stream, "Position", m_memory.m_localPosition);

	m_currentView.SetRadius(newRadius);
	for (int i = -newRadius; i <= newRadius; i++)
	{
		for (int j = -newRadius; j <= newRadius; j++)
		{
			bool visible = Serialization::Read<PackedStream, bool>(stream, "Visible");
			m_currentView.SetVisibilityLocal(i, j, visible);

			if (visible)
			{
				if (Serialization::Read<PackedStream, bool>(stream, "Update Tile"))
				{ 
					ReadTileUpdate(stream, i, j);
				}
			}
		}
	}
}

BackingTile& PlayerData::GetTileForLocal(int x, int y)
{
	Tile& tile = m_memory.GetTileByLocal(x, y);
	return m_backingTiles[tile.m_backingTile];
}

void PlayerData::WriteTileUpdate(PackedStream& stream, int x, int y, Tile& tile)
{
	THandle<BackingTile> backing = tile.m_backingTile;
	ASSERT(backing.IsValid());
	//TODO: Update this to handle backing tile updates
	Serialization::Write(stream, "Handle", backing);

	bool updateBacking = !m_backingTiles.contains(backing);// || backing.GetReference() != m_backingTiles[backing];
	Serialization::Write(stream, "Update backing", updateBacking);
	if (updateBacking)
	{
		m_backingTiles[backing] = backing.GetReference();
		Serialization::Write(stream, "Backing", backing.GetReference());
	}
}

void PlayerData::ReadTileUpdate(PackedStream& stream, int x, int y)
{
	Tile tile;
	Serialization::Read(stream, "Handle", tile.m_backingTile);
	ASSERT(tile.m_backingTile.IsValid());
	if (Serialization::Read<PackedStream, bool>(stream, "Update backing"))
	{
		m_backingTiles[tile.m_backingTile] = Serialization::Read<PackedStream, BackingTile>(stream, "Backing");
	}

	m_memory.SetTileByLocal(x, y, tile);
}