#include "Game.h"
#include "Debug/Profiling.h"
#include "Data/JobSystem.h"
#include "Data/RogueDataManager.h"
#include "Data/RegisterSaveTypes.h"
#include "Core/Materials/Materials.h"
#include "Map/Map.h"
#include "LOS/TileMemory.h"

thread_local Game* Game::game = nullptr;
thread_local RogueDataManager* Game::dataManager = nullptr;
thread_local MaterialManager* Game::materialManager = nullptr;
thread_local StatManager* Game::statManager = nullptr;

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

void Game::AddOutput(Output output)
{
	std::lock_guard lock(outputMutex);
	m_outputs.push_back(output);
}

bool Game::HasNextOutput()
{
	std::lock_guard lock(outputMutex);
	return m_outputs.size() > 0;
}

Output Game::PopNextOutput()
{
	ASSERT(HasNextOutput());
	std::lock_guard lock(outputMutex);
	Output result = m_outputs[0];
	m_outputs.erase(m_outputs.begin());
	return result;
}

void Game::Save(std::string filename)
{
	ROGUE_PROFILE_SECTION("Save File");
	RogueSaveManager::OpenWriteSaveFile(filename);
	dataManager->SaveAll();
	RogueSaveManager::Write("PlayerLoc", playerLoc);
	RogueSaveManager::Write("LookDir", lookDirection);
	RogueSaveManager::Write("LOS", los);
	RogueSaveManager::Write("PlayerData", m_playerData);
	RogueSaveManager::Write("Map", m_currentMap);
	RogueSaveManager::CloseWriteSaveFile();
}

void Game::Load(std::string filename)
{
	ROGUE_PROFILE_SECTION("Load File");
	if (RogueSaveManager::OpenReadSaveFile(filename))
	{
		//RogueSaveManager::Write("View", m_view);
		dataManager->LoadAll();
		RogueSaveManager::Read("PlayerLoc", playerLoc);
		RogueSaveManager::Read("LookDir", lookDirection);
		RogueSaveManager::Read("LOS", los);
		RogueSaveManager::Read("PlayerData", m_playerData);
		RogueSaveManager::Read("Map", m_currentMap);
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
	Game::dataManager->RegisterArena<StatContainer>(200);
	Game::dataManager->RegisterArena<ChunkMap>(1);

	Game::statManager = new StatManager();
	statManager->Init();

	Game::materialManager = new MaterialManager();
	materialManager->Init();

	testMap = dataManager->Allocate<ChunkMap>();

	Vec2 mapSize = Vec2(128, 128);
	THandle<Map> map;

	for (int i = 0; i < 1; i++)
	{
		MaterialContainer groundMat;
		groundMat.AddMaterial("Stone", 1000);

		MaterialContainer mudMat;
		mudMat.AddMaterial("Mud", 1000);

		MaterialContainer airMat(true);
		airMat.AddMaterial("Air", 1);

		MaterialContainer woodWallMat(true);
		woodWallMat.AddMaterial("Wood", 1000, true);

		MaterialContainer stoneWallMat(true);
		stoneWallMat.AddMaterial("Stone", 1000, true);

		map = dataManager->Allocate<Map>(mapSize, i, -20.0f, 2);
		map->LinkBackingTile<BackingTile>(groundMat, woodWallMat);
		map->LinkBackingTile<BackingTile>(mudMat, airMat);
		map->LinkBackingTile<BackingTile>(groundMat, airMat);
		map->LinkBackingTile<BackingTile>(groundMat, stoneWallMat);

		testMap->LinkBackingTile<BackingTile>(groundMat, woodWallMat);
		testMap->LinkBackingTile<BackingTile>(mudMat, airMat);
		testMap->LinkBackingTile<BackingTile>(groundMat, airMat);
		testMap->LinkBackingTile<BackingTile>(groundMat, stoneWallMat);

		map->FillTilesExc(Vec2(0, 0), mapSize, 0);
		map->FillTilesExc(Vec2(1, 1), Vec2(mapSize.x - 1, mapSize.y - 1), 1);

		std::srand(std::time({}));
		for (int i = 0; i < 100; i++)
		{
			int r = (std::rand() % 5) + 1;
			int x = std::rand() % (mapSize.x - r);
			int y = std::rand() % (mapSize.y - r);

			map->FillTilesExc(Vec2(x, y), Vec2(x + r, y + r), 0);
		}

		for (int i = 0; i < 100; i++)
		{
			int r = (std::rand() % 5) + 1;
			int x = std::rand() % (mapSize.x - r);
			int y = std::rand() % (mapSize.y - r);

			map->FillTilesExc(Vec2(x, y), Vec2(x + r, y + r), 3);
		}
	}

	m_currentMap = map;
	m_playerData.GetCurrentMemory() = TileMemory(map);
	m_playerData.GetCurrentMemory().Move(Vec2(1, 1));
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

		while (active && HasNextInput())
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
	delete Game::statManager;
}

void Game::HandleInput(const Input& input)
{
	switch (input.m_type)
	{
	case EInputType::InvalidInput:
		break;
	case EInputType::Movement:
		{
			ROGUE_PROFILE_SECTION("Handle Movement Input");
			std::shared_ptr<TInput<Movement>> data = input.Get<Movement>();
			auto move = playerLoc.Traverse(data->m_direction, lookDirection);
			playerLoc = move.first;
			lookDirection = Rotate(lookDirection, move.second);
			m_playerData.GetCurrentMemory().Move(Vector2FromDirection(data->m_direction));

			//m_currentMap->Simulate();

			//auto data = input.Get<EInputType::Movement>();
			los.SetRadius(30);
			LOS::Calculate(los, playerLoc, lookDirection);
			m_playerData.UpdateViewGame(los);

			testMap->Simulate(playerLoc);
			testMap->TriggerStreamingAroundLocation(playerLoc);
		}
		break;
	case EInputType::DEBUG_FIRE:
	{
		testMap->AddHeat(playerLoc, 1000);
	}
	case EInputType::Wait:
		{
			//m_currentMap->Simulate();
			testMap->Simulate(playerLoc);

			//auto data = input.Get<EInputType::Movement>();
			los.SetRadius(30);
			LOS::Calculate(los, playerLoc, lookDirection);
			m_playerData.UpdateViewGame(los);
		}
		break;
	case EInputType::DEBUG_MAKE_STONE:
		testMap->SetTile(playerLoc, 3);
		break;
	case EInputType::BeginNewGame:
		InitNewGame();

		testMap->TriggerStreamingAroundLocation(playerLoc);
		//Assumably, we just enqueued a ton of jobs to get the game spun up - let that queue clear out before we start
		testMap->WaitForStreaming();

		//Game ready! Temp - boot up first view
		los.SetRadius(30);
		LOS::Calculate(los, playerLoc, lookDirection);
		m_playerData.UpdateViewGame(los);

		CreateOutput<GameReady>();
		break;
	case EInputType::BeginSeededGame:
		break;
	case EInputType::LoadSaveGame:
		{
			InitNewGame();
			auto data = input.Get<LoadSaveGame>();
			Load(data->fileName);
			m_playerData.UpdateViewGame(los);
			CreateOutput<GameReady>();
		}
		break;
	case EInputType::SaveAndExit:
		Save("TestSave.rsf");
		active = false;
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