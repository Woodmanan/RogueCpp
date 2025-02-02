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

private:
	std::map<THandle<BackingTile>, BackingTile> m_backingTiles;
	TileMemory m_memory;
	View m_currentView;
};