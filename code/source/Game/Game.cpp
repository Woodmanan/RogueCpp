#include "Game.h"
#include "Debug/Profiling.h"

thread_local Game* Game::game = nullptr;

Game::Game()
{

}

void Game::LaunchGame()
{
	MainLoop();
}

void Game::AddInput(Input input)
{
	{
		std::lock_guard lock(inputMutex);
		m_inputs.push_back(input);
	}
	inputCv.notify_all();
}

//Setup a fresh game
void Game::InitNewGame()
{
	Game::game = this;
}

void Game::MainLoop()
{
	ROGUE_PROFILE;
	while (active)
	{
		//Hand over OS control of lock - keeps thread efficiently sleeping until someone wants to pass it info
		{
			std::unique_lock lock(inputMutex);
			inputCv.wait(lock, [this] { return m_inputs.size() > 0; });
		}

		while (HasNextInput())
		{
			ROGUE_PROFILE_SECTION("Game loop Step");
			HandleInput(PopNextInput());
		}
	}
}

void Game::HandleInput(const Input& input)
{
	switch (input.m_type)
	{
	case EInputType::Invalid:
		break;
	case EInputType::Movement:
		break;
	case EInputType::BeginNewGame:
		InitNewGame();
		break;
	case EInputType::BeginSeededGame:
		break;
	case EInputType::LoadSaveGame:
		break;
	case EInputType::SaveAndExit:
		break;
	case EInputType::ExitGame:
		active = false;
		break;
	}
}

bool Game::HasNextInput()
{
	std::lock_guard lock(inputMutex);
	return m_inputs.size() > 0;
}

Input Game::PopNextInput()
{
	ASSERT(HasNextInput());
	std::lock_guard lock(inputMutex);
	Input result = m_inputs[0];
	m_inputs.erase(m_inputs.begin());
	return result;
}