#include "Core/CoreDataTypes.h"
#include "Game/IO.h"
#include <filesystem>
#include <vector>
#include <mutex>

/*
 * Big game state! This represents a thread-specific black-box which is runnign
 */

class Game
{
public:
	Game();

	void LaunchGame();

	void AddInput(Input input);

	//Game state
	static thread_local Game* game;
	bool active = true;

private:
	void InitNewGame();
	void MainLoop();

	void HandleInput(const Input& input);

	bool HasNextInput();
	Input PopNextInput();

	//IO Handling
	std::mutex inputMutex;
	std::condition_variable inputCv;
	std::vector<Input> m_inputs;
};