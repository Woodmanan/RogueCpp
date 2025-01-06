#include "Game.h"
#include "Debug/Profiling.h"
#include "Data/RogueDataManager.h"
#include "Data/RegisterSaveTypes.h"
#include "Core/Materials/Materials.h"
#include "Map/Map.h"
#include "LOS/TileMemory.h"

thread_local Game* Game::game = nullptr;
thread_local RogueDataManager* Game::dataManager = nullptr;
thread_local MaterialManager* Game::materialManager = nullptr;

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

void Game::Save(std::string filename)
{
	ROGUE_PROFILE_SECTION("Save File");
	RogueSaveManager::OpenWriteSaveFile(filename);
	dataManager->SaveAll();
	RogueSaveManager::CloseWriteSaveFile();
}

void Game::Load(std::string filename)
{
	ROGUE_PROFILE_SECTION("Load File");
	if (RogueSaveManager::OpenReadSaveFile(filename))
	{
		dataManager->LoadAll();
		RogueSaveManager::CloseReadSaveFile();
	}
}

//Setup a fresh game
void Game::InitNewGame()
{
	Game::game = this;
	Game::dataManager = new RogueDataManager();
	Game::dataManager->RegisterArena<BackingTile>(20);
	Game::dataManager->RegisterArena<TileStats>(20);
	Game::dataManager->RegisterArena<Map>(20);
	Game::dataManager->RegisterArena<TileNeighbors>(20);
	Game::dataManager->RegisterArena<TileMemory>(20);
	Game::dataManager->RegisterArena<MaterialContainer>(20);

	Game::materialManager = new MaterialManager();
	materialManager->Init();

	Vec2 mapSize = Vec2(64, 64);

	for (int i = 0; i < 1; i++)
	{
		MaterialContainer groundMat;
		groundMat.AddMaterial("Dirt", 1000);
		MaterialContainer wallMat;
		wallMat.AddMaterial("Dirt", 1000);
		wallMat.AddMaterial("Stone", 1000, true);

		THandle<Map> map = dataManager->Allocate<Map>(mapSize, i, 2);
		map->LinkBackingTile<BackingTile>('#', Color(120, 120, 120), Color(0, 0, 0), true, -1, wallMat);
		map->LinkBackingTile<BackingTile>('.', Color(100, 200, 100), Color(0, 0, 0), false, 1, groundMat);
		map->LinkBackingTile<BackingTile>('S', Color(200, 100, 200), Color(80, 80, 80), false, 1, groundMat);
		map->LinkBackingTile<BackingTile>('0', Color(60, 60, 255), Color(0, 0, 0), false, 1, groundMat);
		map->LinkBackingTile<BackingTile>('1', Color(60, 60, 255), Color(0, 0, 0), false, 1, groundMat);
		map->LinkBackingTile<BackingTile>('2', Color(60, 60, 255), Color(0, 0, 0), false, 1, groundMat);
		map->LinkBackingTile<BackingTile>('3', Color(60, 60, 255), Color(0, 0, 0), false, 1, groundMat);
		map->LinkBackingTile<BackingTile>('4', Color(60, 60, 255), Color(0, 0, 0), false, 1, groundMat);
		map->LinkBackingTile<BackingTile>('5', Color(60, 60, 255), Color(0, 0, 0), false, 1, groundMat);

		map->FillTilesExc(Vec2(0, 0), mapSize, 0);
		map->FillTilesExc(Vec2(1, 1), Vec2(mapSize.x - 1, mapSize.y - 1), 1);

		THandle<TileMemory> memory = dataManager->Allocate<TileMemory>(map);
		memory->SetLocalPosition(Location(0,0,0));
	}
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

	Cleanup();
}

void Game::Cleanup()
{
	delete Game::dataManager;
	delete Game::materialManager;
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