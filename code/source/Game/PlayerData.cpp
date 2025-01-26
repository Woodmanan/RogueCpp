#include "PlayerData.h"
#include "Data/Serialization/Serialization.h"

void PlayerData::UpdateViewGame(View& newView)
{
	DefaultStream beforeStream;
	DefaultStream afterStream;

	Serialization::Write(beforeStream, "View", m_currentView);
	Serialization::Write(afterStream, "View", m_currentView);

	m_currentView = newView;
}

void PlayerData::UpdateViewPlayer(DefaultStream& stream)
{
	Serialization::Read(stream, "View", m_currentView);
}