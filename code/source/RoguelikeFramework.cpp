// RoguelikeFramework.cpp : Defines the entry point for the application.
//

#include "RoguelikeFramework.h"
#include "Data/RogueArena.h"
#include "..\libraries\BearLibTerminal\Include\C\BearLibTerminal.h"
#include "Data/RogueDataManager.h"
#include "Data/SaveManager.h"
#include "Data/RegisterSaveTypes.h"
#include "Map/Map.h"
#include <format>
#include <chrono>

using namespace std;

void RenderMap(THandle<Map> map, Vec2 size, Vec2 window, Location playerLocation)
{
    for (int i = 0; i < window.x; i++)
    {
        for (int j = 0; j < window.y; j++)
        {
            Vec2 playerOffset = Vec2(i, window.y - j) - Vec2(40, 20);
            Location mapLocation = playerLocation + playerOffset;
            if (mapLocation.InMap())
            {
                terminal_color(mapLocation->m_backingTile->m_foregroundColor);
                terminal_bkcolor(mapLocation->m_backingTile->m_backgroundColor);
                terminal_put(i, j, mapLocation->m_backingTile->m_renderCharacter);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    //Initialize Random
    srand(1);

    Vec2 size(64, 64);
    THandle<Map> map;

    for (int i = 0; i < 1; i++)
    {
        map = RogueDataManager::Allocate<Map>(size, i, 2);
        map->LinkBackingTile<BackingTile>('#', color_from_argb(255, 120, 120, 120), color_from_argb(255, 0, 0, 0), true, -1);
        map->LinkBackingTile<BackingTile>('.', color_from_argb(255, 200, 200, 200), color_from_argb(255, 0, 0, 0), false, 1);

        map->FillTilesExc(Vec2(0, 0), size, 1);
        map->FillTilesExc(Vec2(1, 1), Vec2(size.x - 1, size.y - 1), 1);

        for (int j = 0; j < 1000; j++)
        {
            Vec2 location = Vec2(rand() % map->m_size.x, rand() % map->m_size.y);
            map->SetTile(location, 0);
        }
    }

    map->WrapMapEdges();

    Location start = Location(0, 0, 0);
    Location next = Location(1, 1, 0);
    Location invalid;
    bool testBit;

    //RogueSaveManager::OpenWriteSaveFile("MySaveFile.rsf");
    //RogueSaveManager::Write("Map", map);
    //RogueSaveManager::Write("Start", start);
    //RogueSaveManager::Write("next", next);
    //RogueSaveManager::Write("invalid", invalid);
    //RogueDataManager::Get()->SaveAll();
    //RogueSaveManager::CloseWriteSaveFile();

    //RogueSaveManager::OpenReadSaveFile("MySaveFile.rsf");
    //RogueSaveManager::Read("Map", map);
    //RogueSaveManager::Read("Start", start);
    //RogueSaveManager::Read("next", next);
    //RogueSaveManager::Read("invalid", invalid);
    //RogueDataManager::Get()->LoadAll();
    //RogueSaveManager::CloseReadSaveFile();

    Location playerLoc = Location(1, 1, 0);

    char k;
    int x = 80, y = 40;
    int pos_x = 40;
    int pos_y = 20;
    terminal_open();
    terminal_set("window: size=81x41");
    terminal_print(34, 20, "hello, world! I am here");
    //terminal_print(34, 21, std::to_string(test.GetID()).c_str());
    terminal_refresh();

    auto clock = chrono::system_clock::now();
    float FPS = 0.0f;

    bool shouldBreak = false;
    while (!shouldBreak) {
        if (terminal_has_input())
        {
            k = terminal_read();
        }
        switch (k)
        {
        case TK_Q:
            shouldBreak = true;
            break;
        case TK_H:
        case TK_LEFT:
            if (terminal_state(TK_SHIFT))
            {
                for (int i = 0; i < 10; i++)
                {
                    playerLoc = playerLoc.Traverse(Direction::West);
                }
            }
            else
            {
                playerLoc = playerLoc.Traverse(Direction::West);
            }
            break;
        case TK_K:
        case TK_UP:
            if (terminal_state(TK_SHIFT))
            {
                for (int i = 0; i < 10; i++)
                {
                    playerLoc = playerLoc.Traverse(Direction::North);
                }
            }
            else
            {
                playerLoc = playerLoc.Traverse(Direction::North);
            }
            break;
        case TK_J:
        case TK_DOWN:
            if (terminal_state(TK_SHIFT))
            {
                for (int i = 0; i < 10; i++)
                {
                    playerLoc = playerLoc.Traverse(Direction::South);
                }
            }
            else
            {
                playerLoc = playerLoc.Traverse(Direction::South);
            }
            break;
        case TK_L:
        case TK_RIGHT:
            if (terminal_state(TK_SHIFT))
            {
                for (int i = 0; i < 10; i++)
                {
                    playerLoc = playerLoc.Traverse(Direction::East);
                }
            }
            else
            {
                playerLoc = playerLoc.Traverse(Direction::East);
            }
            break;
        case TK_Y:
            playerLoc = playerLoc.Traverse(Direction::NorthWest);
            break;
        case TK_U:
            playerLoc = playerLoc.Traverse(Direction::NorthEast);
            break;
        case TK_B:
            playerLoc = playerLoc.Traverse(Direction::SouthWest);
            break;
        case TK_N:
            playerLoc = playerLoc.Traverse(Direction::SouthEast);
            break;
        }

        k = TK_G;
        terminal_clear();
        RenderMap(map, size, Vec2(x, y), playerLoc);
        terminal_put(40, 20, '@');


        char buffer[10];
        std::snprintf(&buffer[0], 10, "FPS: %f", FPS);
        terminal_print(0, 0, &buffer[0]);

        clock = chrono::system_clock::now();
        terminal_refresh();
        auto currentTime = chrono::system_clock::now();
        auto frameTime = chrono::duration_cast<chrono::duration<float>>(currentTime - clock);
        clock = currentTime;

        FPS = 1.0f / (frameTime.count());
    }
    terminal_close();

    return EXIT_SUCCESS;
}
