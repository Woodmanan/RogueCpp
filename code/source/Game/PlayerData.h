#pragma once
#include "GameHeaders.h"
#include "IO.h"
#include "Data/Serialization/BitStream.h"

class PlayerData
{
public:
	View& GetCurrentView() { return m_currentView; }
	void UpdateViewGame(View& newView);
	void UpdateViewPlayer(std::shared_ptr<TOutput<ViewUpdated>> updated);
	BackingTile& GetTileFor(Location location);

private:
	std::map<THandle<BackingTile>, BackingTile> m_backingTiles;
	std::map<Location, THandle<BackingTile>> m_tileData;
	View m_currentView;
};