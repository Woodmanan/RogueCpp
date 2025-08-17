#pragma once
#include "GameHeaders.h"
#include "IO.h"
#include "Data/Serialization/BitStream.h"

class PlayerData
{
public:
	PlayerData();

	View& GetCurrentView() { return m_currentView; }
	TileMemory& GetCurrentMemory() { return m_memory; }
	void UpdateViewGame(View& newView);
	void UpdateViewPlayer(std::shared_ptr<TOutput<ViewUpdated>> updated);
	BackingTile& GetTileForLocal(int x, int y);

	std::map<THandle<BackingTile>, BackingTile> m_backingTiles;
	TileMemory m_memory;
	View m_currentView;
};

namespace Serialization
{
    template<typename Stream>
    void Serialize(Stream& stream, const PlayerData& value)
    {
        Write(stream, "BackingTiles", value.m_backingTiles);
        Write(stream, "Memory", value.m_memory);
        Write(stream, "View", value.m_currentView);
    }

    template <typename Stream>
    void Deserialize(Stream& stream, PlayerData& value)
    {
        Read(stream, "BackingTiles", value.m_backingTiles);
        Read(stream, "Memory", value.m_memory);
        Read(stream, "View", value.m_currentView);
    }
}