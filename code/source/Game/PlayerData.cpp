#include "PlayerData.h"
#include "Data/Serialization/Serialization.h"
#include "Game/Game.h"
#include "LOS/TileMemory.h"

PlayerData::PlayerData()
{
	BackingTile emptyTile;
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
					WriteTileUpdate(afterStream, i, j, m_memory.GetTileByLocal(i, j), DataTile::FromTile(loc.GetTile()));
				}
			}
		}
	}

	m_memory.Update(newView);

	afterStream.AllWritesFinished();
	std::shared_ptr<VectorBackend> backend = dynamic_pointer_cast<VectorBackend>(afterStream.GetDataBackend());
	DEBUG_PRINT("View bytes: %d", backend->m_data.size());

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

DataTile& PlayerData::GetTileForLocal(int x, int y)
{
	return m_memory.GetTileByLocal(x, y);
}

BackingTile& PlayerData::GetBackingTileForLocal(int x, int y)
{
	DataTile& tile = m_memory.GetTileByLocal(x, y);
	return m_backingTiles[tile.m_backingTile];
}

void PlayerData::WriteTileUpdate(PackedStream& stream, int x, int y, const DataTile& oldTile, const DataTile& newTile)
{
	bool updateHandle = (oldTile.m_backingTile != newTile.m_backingTile);
	Serialization::Write(stream, "Update backing handle", updateHandle);
	THandle<BackingTile> backing = newTile.m_backingTile;
	ASSERT(backing.IsValid());

	if (updateHandle)
	{
		Serialization::Write(stream, "Handle", backing);
	}

	bool updateBacking = !m_backingTiles.contains(backing);// || backing.GetReference() != m_backingTiles[backing];
	Serialization::Write(stream, "Update backing", updateBacking);
	if (updateBacking)
	{
		m_backingTiles[backing] = backing.GetReference();
		Serialization::Write(stream, "Backing", backing.GetReference());
	}

	Serialization::Write(stream, "Temperature", newTile.m_temperature);

	bool updateColor = oldTile.m_color != newTile.m_color || oldTile.m_renderChar != newTile.m_renderChar;
	Serialization::Write(stream, "Update color", updateColor);
	if (updateColor)
	{
		Serialization::Write(stream, "Char", newTile.m_renderChar);
		Serialization::Write(stream, "Color", newTile.m_color);
	}
}

void PlayerData::ReadTileUpdate(PackedStream& stream, int x, int y)
{
	DataTile tile = m_memory.GetTileByLocal(x, y);

	if (Serialization::Read<PackedStream, bool>(stream, "Update backing handle"))
	{
		Serialization::Read(stream, "Handle", tile.m_backingTile);
		ASSERT(tile.m_backingTile.IsValid());
	}

	if (Serialization::Read<PackedStream, bool>(stream, "Update backing"))
	{
		m_backingTiles[tile.m_backingTile] = Serialization::Read<PackedStream, BackingTile>(stream, "Backing");
	}

	Serialization::Read(stream, "Temperature", tile.m_temperature);

	if (Serialization::Read<PackedStream, bool>(stream, "Update color"))
	{
		Serialization::Read(stream, "Char", tile.m_renderChar);
		Serialization::Read(stream, "Color", tile.m_color);
	}

	m_memory.SetTileByLocal(x, y, tile);
}