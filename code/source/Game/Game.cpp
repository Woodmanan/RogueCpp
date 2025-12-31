#include "Game.h"
#include "Debug/Profiling.h"
#include "Data/JobSystem.h"
#include "Data/RogueDataManager.h"
#include "Data/RegisterSaveTypes.h"
#include "Core/Materials/Materials.h"
#include "Map/Map.h"
#include "Map/WorldManager.h"
#include "LOS/TileMemory.h"
#include "Core/Pathfinding/Pathfinding.h"
#include "Utils/Utils.h"

thread_local Game* Game::game = nullptr;
thread_local RogueDataManager* Game::dataManager = nullptr;
thread_local MaterialManager* Game::materialManager = nullptr;
thread_local StatManager* Game::statManager = nullptr;
thread_local WorldManager* Game::worldManager = nullptr;

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
	RogueSaveManager::Write("Seed", m_seed);
	dataManager->SaveAll();
	RogueSaveManager::Write("Player", m_player);
	RogueSaveManager::Write("PlayerData", m_playerData);
	RogueSaveManager::CloseWriteSaveFile();
}

void Game::Load(std::string filename)
{
	ROGUE_PROFILE_SECTION("Load File");
	if (RogueSaveManager::OpenReadSaveFile(filename))
	{
		RogueSaveManager::Read("Seed", m_seed);
		//RogueSaveManager::Write("View", m_view);
		dataManager->LoadAll();
		RogueSaveManager::Read("Player", m_player);
		RogueSaveManager::Read("PlayerData", m_playerData);
		RogueSaveManager::CloseReadSaveFile();
	}
}

//Setup a fresh game
void Game::InitNewGame(uint seed)
{
	if (seed == 0)
	{
		srand(time(nullptr));
		m_seed = rand();
	}
	else
	{
		m_seed = seed;
	}

	Game::game = this;
	Game::dataManager = new RogueDataManager();
	Game::dataManager->RegisterArena<BackingTile>(20);
	Game::dataManager->RegisterArena<TileStats>(200);
	Game::dataManager->RegisterArena<ChunkMap>(1);
	Game::dataManager->RegisterArena<TileNeighbors>(200);
	Game::dataManager->RegisterArena<TileMemory>(1);
	Game::dataManager->RegisterArena<MaterialContainer>(20);
	Game::dataManager->RegisterArena<StatContainer>(20);
	Game::dataManager->RegisterArena<Monster>(200);

	Game::statManager = new StatManager();
	statManager->Init();

	Game::materialManager = new MaterialManager();
	materialManager->Init();

	Game::worldManager = new WorldManager();
	worldManager->Init();

	map = dataManager->Allocate<ChunkMap>();

	{ // Backing tile linkage - TODO: Make this automatic!!
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

		map->LinkBackingTile<BackingTile>(groundMat, woodWallMat);
		map->LinkBackingTile<BackingTile>(mudMat, airMat);
		map->LinkBackingTile<BackingTile>(groundMat, airMat);
		map->LinkBackingTile<BackingTile>(groundMat, stoneWallMat);
	}

	m_player = dataManager->Allocate<Monster>(ResourceManager::Get()->LoadSynchronous("MonsterDefinition", "Player"));
	m_player->SetLocation(Location(0, 0, 0));

	m_playerData.GetCurrentMemory() = TileMemory();
	//m_playerData.GetCurrentMemory().Move(Vec2(1, 1));
}

void Game::MainLoop()
{
	std::string name = string_format("Game thread");
	ROGUE_NAME_THREAD(name.c_str());

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
		Direction offset = input.Get<Movement>()->m_direction;
		Location next = m_player->GetLocation().Traverse(offset, m_player->GetRotation()).first;

		STACKARRAY(Location, locations, 5);
		Pathfinding::PathfindingSettings settings;
		settings.m_maxCost = 5;
		settings.positionGenerator = GetMember(m_player, &Monster::GetAllowedMovements);
		if (Pathfinding::GetPath(m_player->GetLocation(), next, settings, locations) && locations.size() == 2)
		{
			m_player->Move(offset);
			m_playerData.GetCurrentMemory().Move(Vector2FromDirection(offset));

			m_player->GetView().SetRadius(30);
			LOS::Calculate(m_player);
			m_playerData.UpdateViewGame(m_player->GetView());

			map->Simulate(m_player->GetLocation());
			map->TriggerStreamingAroundLocation(m_player->GetLocation());
		}
	}
	break;
	case EInputType::DEBUG_FIRE:
	{
		map->AddHeat(m_player->GetLocation(), 1000);
	}
	case EInputType::Wait:
	{
		map->Simulate(m_player->GetLocation());

		m_player->GetView().SetRadius(30);
		LOS::Calculate(m_player);
		m_playerData.UpdateViewGame(m_player->GetView());
	}
	break;
	case EInputType::DEBUG_MAKE_STONE:
		map->SetTile(m_player->GetLocation(), 3);
		break;
	case EInputType::BeginNewGame:
		InitNewGame();

		map->TriggerStreamingAroundLocation(m_player->GetLocation());
		//Assumably, we just enqueued a ton of jobs to get the game spun up - let that queue clear out before we start
		map->WaitForStreaming();

		//Game ready! Temp - boot up first view
		m_player->GetView().SetRadius(30);
		LOS::Calculate(m_player);
		m_playerData.UpdateViewGame(m_player->GetView());

		CreateOutput<GameReady>();
		break;
	case EInputType::BeginSeededGame:
	{
		auto data = input.Get<BeginSeededGame>();
		InitNewGame(data->seed);
		map->TriggerStreamingAroundLocation(m_player->GetLocation());
		//Assumably, we just enqueued a ton of jobs to get the game spun up - let that queue clear out before we start
		map->WaitForStreaming();

		//Game ready! Temp - boot up first view
		m_player->GetView().SetRadius(30);
		LOS::Calculate(m_player);
		m_playerData.UpdateViewGame(m_player->GetView());

		CreateOutput<GameReady>();
	}
	break;
	case EInputType::LoadSaveGame:
	{
		InitNewGame();
		auto data = input.Get<LoadSaveGame>();
		Load(data->fileName);
		m_playerData.UpdateViewGame(m_player->GetView());
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
	case EInputType::RequestPath:
		{
			Vec3 offset = input.Get<RequestPath>()->m_localOffset;
			Location playerLoc = m_player->GetLocation();
			Location offsetLoc = Location(playerLoc.GetVector() + offset);

			STACKARRAY(Location, locations, 60);
			Pathfinding::PathfindingSettings settings;
			settings.m_maxCost = 30;
			settings.positionGenerator = GetMember(m_player, &Monster::GetAllowedMovements);
			if (Pathfinding::GetPath(playerLoc, offsetLoc, settings, locations))
			{
				vector<Vec2> offsets;
				for (Location location : locations)
				{
					Vec2 diff = location.AsVec2() - playerLoc.AsVec2();
					offsets.push_back(diff);
				}
				CreateOutput<RecievePath>(offsets);
			}
			else
			{
				CreateOutput<RecievePath>(vector<Vec2>());
			}
		}
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