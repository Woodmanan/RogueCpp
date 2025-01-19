#pragma once
#include "GameHeaders.h"
#include "IO.h"

/*
 * Big game state! This represents a thread-specific black-box which is runnign
 */

class Game
{
public:
	Game();

	//Game state
	static thread_local Game* game;
	static thread_local RogueDataManager* dataManager;
	static thread_local MaterialManager* materialManager;

	bool active = true;

	void LaunchGame();
	void AddInput(Input input);

	template <typename T, class... Args>
	void CreateInput(Args&&... args)
	{
		Input input;
		std::shared_ptr<T> ptr = std::make_shared<T>(std::forward<Args>(args)...);
		input.Set<T>(ptr);
		AddInput(input);
	}

	template <EInputType inputType>
	void CreateInput()
	{
		Input input;
		input.Set<inputType>();
		AddInput(input);
	}

	void Save(std::string filename);
	void Load(std::string filename);

private:
	void InitNewGame();
	void MainLoop();
	void Cleanup();

	void HandleInput(const Input& input);

	bool HasNextInput();
	Input PopNextInput();

	//IO Handling
	std::mutex inputMutex;
	std::condition_variable inputCv;
	std::vector<Input> m_inputs;

	View los;
};