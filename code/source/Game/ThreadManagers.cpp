#include "ThreadManagers.h"
#include "Game.h"

Game* GetGame()
{
	return Game::game;
}

RogueDataManager* GetDataManager()
{
	return Game::dataManager;
}

MaterialManager* GetMaterialManager()
{
	return Game::materialManager;
}