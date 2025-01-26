#include "GameHeaders.h"
#include "IO.h"
#include "Data/Serialization/BitStream.h"

class PlayerData
{
public:
	View& GetCurrentView() { return m_currentView; }
	void UpdateViewGame(View& newView);
	void UpdateViewPlayer(DefaultStream& stream);

private:
	View m_currentView;
};