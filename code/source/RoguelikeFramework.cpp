// RoguelikeFramework.cpp : Defines the entry point for the application.
//

#include "RoguelikeFramework.h"
#include "Data/RogueArena.h"
#include "..\libraries\BearLibTerminal\Include\C\BearLibTerminal.h"
#include "Data/RogueDataManager.h"
#include "Data/SaveManager.h"
#include "Data/RegisterSaveTypes.h"
#include "Map/Map.h"
#include "LOS/LOS.h"
#include <format>
#include <chrono>

using namespace std;

color_t passColors[5] = {
    color_from_argb(255, 255, 0, 255),
    color_from_argb(255, 0, 200, 0),
    color_from_argb(255, 100, 100, 200),
    color_from_argb(255, 200, 200, 00),
    color_from_argb(255, 255, 100, 100),
};

color_t Blend(color_t a, color_t b)
{
    uint32_t alpha = (((a >> 24) & 0xFF) + ((b >> 24) & 0xFF)) / 2 & 0xFF;
    uint32_t red =   (((a >> 16) & 0xFF) + ((b >> 16) & 0xFF)) / 2 & 0xFF;
    uint32_t green = (((a >>  8) & 0xFF) + ((b >>  8) & 0xFF)) / 2 & 0xFF;
    uint32_t blue =  (((a >>  0) & 0xFF) + ((b >>  0) & 0xFF)) / 2 & 0xFF;
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

void RenderMap(THandle<Map> map, Vec2 size, Vec2 window, Location playerLocation, View& view)
{
    for (int i = 0; i < window.x; i++)
    {
        for (int j = 0; j < window.y; j++)
        {
            Vec2 playerOffset = Vec2(i, window.y - j) - Vec2(40, 20);
            Location mapLocation = playerLocation + playerOffset;
            if (mapLocation.GetValid() && mapLocation.InMap())
            {
                terminal_color(mapLocation->m_backingTile->m_foregroundColor);
                terminal_bkcolor(mapLocation->m_backingTile->m_backgroundColor);
                terminal_put(i, j, mapLocation->m_backingTile->m_renderCharacter);
            }
        }
    }

    int maxRadius = std::min(view.GetRadius(), (int)window.x);

    for (int i = -maxRadius; i <= maxRadius; i++)
    {
        for (int j = -maxRadius; j <= maxRadius; j++)
        {
            Vec2 windowCoord = Vec2(i, -j) + Vec2(40, 20);

            bool visible = view.GetVisibilityLocal(i, j);
            if (view.GetLocationLocal(i, j).GetValid())
            {
                if (visible)
                {
                    int step = view.GetVisibilityPassIndex(i, j);
                    color_t color = passColors[step % 5];
                    terminal_color(color);
                    //terminal_color(Blend(view.GetLocationLocal(i, j)->m_backingTile->m_foregroundColor, green));
                }
                else
                {
                    terminal_color(color_from_argb(255, 0, 0, 0));
                }
                terminal_bkcolor(color_from_argb(255, 0, 0, 0));
                terminal_put(windowCoord.x, windowCoord.y, view.GetLocationLocal(i, j)->m_backingTile->m_renderCharacter);
            }
            else
            {
                terminal_color(color_from_argb(255, 0, 0, 0));
                terminal_bkcolor(color_from_argb(255, 0, 0, 0));
                terminal_put(windowCoord.x, windowCoord.y, ' ');
            }
        }
    }
}

void RenderBresenham(Vec2 window, Vec2 endpoint)
{
    terminal_color(color_from_argb(255, 100, 0, 100));
    terminal_bkcolor(color_from_argb(255, 0, 0, 0));
    int x0 = 0;
    int y0 = 0;
    int dx = abs(endpoint.x - 0), sx = 0 < endpoint.x ? 1 : -1;
    int dy = -abs(endpoint.y - 0), sy = 0 < endpoint.y ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */

    for (;;) {  /* loop */
        Vec2 pos = Vec2(40, 20) + Vec2(x0, -y0);
        terminal_put(pos.x, pos.y, '*');
        if (x0 == endpoint.x && y0 == endpoint.y)
        {
            terminal_put(pos.x, pos.y, '+');
            break;
        }
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
    }
}

void ResetMap(THandle<Map> map)
{
    if (map.IsValid())
    {

    }
}

int main(int argc, char* argv[])
{
    //Initialize Random
    srand(1);

    Vec2 size(32, 32);
    THandle<Map> map;
    Location playerLoc = Location(1, 1, 0);
    Location warpPosition;
    int radius = 10;
    int maxPass = 10;
    Vec2 bresenhamPoint = Vec2(0, 0);

    if (RogueSaveManager::FileExists("MySaveFile.rsf"))
    {
        RogueSaveManager::OpenReadSaveFile("MySaveFile.rsf");
        RogueSaveManager::Read("Map", map);
        RogueSaveManager::Read("Player", playerLoc);
        RogueSaveManager::Read("Radius", radius);
        RogueSaveManager::Read("Max Pass", maxPass);
        RogueSaveManager::Read("bPoint", bresenhamPoint);
        RogueDataManager::Get()->LoadAll();
        RogueSaveManager::CloseReadSaveFile();
    }
    else
    {
        for (int i = 0; i < 1; i++)
        {
            map = RogueDataManager::Allocate<Map>(size, i, 2);
            map->LinkBackingTile<BackingTile>('#', color_from_argb(255, 120, 120, 120), color_from_argb(255, 0, 0, 0), true, -1);
            map->LinkBackingTile<BackingTile>('.', color_from_argb(255, 200, 200, 200), color_from_argb(255, 0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('S', color_from_argb(255, 200, 100, 200), color_from_argb(255, 80, 80, 80), false, 1);
            map->LinkBackingTile<BackingTile>('0', color_from_argb(255, 60, 60, 255), color_from_argb(255, 0, 0, 0), false, 1);

            map->FillTilesExc(Vec2(0, 0), size, 0);
            map->FillTilesExc(Vec2(1, 1), Vec2(size.x - 1, size.y - 1), 1);
        }

        map->WrapMapEdges();
    }

    char k;
    int x = 80, y = 40;
    int pos_x = 40;
    int pos_y = 20;
    terminal_open();
    terminal_set("window: size=81x41");
    terminal_print(34, 20, "hello, world! I am here");
    //terminal_print(34, 21, std::to_string(test.GetID()).c_str());
    terminal_refresh();

    View view;
    view.SetRadius(radius);
    View fakeBackground;
    fakeBackground.SetRadius(0);

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
            if (terminal_state(TK_SHIFT))
            {
                RogueSaveManager::DeleteSaveFile("MySaveFile.rsf");
            }
            shouldBreak = true;
            break;
        case TK_H:
        case TK_LEFT:
            if (terminal_state(TK_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(-1, 0);
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
                bresenhamPoint = bresenhamPoint + Vec2(0, 1);
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
                bresenhamPoint = bresenhamPoint + Vec2(0, -1);
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
                bresenhamPoint = bresenhamPoint + Vec2(1, 0);
            }
            else
            {
                playerLoc = playerLoc.Traverse(Direction::East);
            }
            break;
        case TK_Y:
            if (terminal_state(TK_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(-1, 1);
            }
            else
            {
                playerLoc = playerLoc.Traverse(Direction::NorthWest);
            }
            break;
        case TK_U:
            if (terminal_state(TK_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(1, 1);
            }
            else
            {
                playerLoc = playerLoc.Traverse(Direction::NorthEast);
            }
            break;
        case TK_B:
            if (terminal_state(TK_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(-1, -1);
            }
            else
            {
                playerLoc = playerLoc.Traverse(Direction::SouthWest);
            }
            break;
        case TK_N:
            if (terminal_state(TK_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(1, -1);
            }
            else
            {
                playerLoc = playerLoc.Traverse(Direction::SouthEast);
            }
            break;
        case TK_SPACE:
            map->SetTile(playerLoc.AsVec2(), !playerLoc->m_backingTile->m_index);
            break;
        case TK_EQUALS:
            if (terminal_state(TK_SHIFT))
            {
                maxPass = maxPass + 1;
            }
            else
            {
                radius = radius + 1;
                view.SetRadius(radius);
            }
            break;
        case TK_MINUS:
            if (terminal_state(TK_SHIFT))
            {
                maxPass = std::max(1, maxPass - 1);
            }
            else
            {
                radius = std::max(0, radius - 1);
                view.SetRadius(radius);
            }
            break;
        case TK_ENTER:
            if (!warpPosition.GetValid())
            {
                warpPosition = playerLoc;
            }
            else
            {
                //Establish the warp!
                map->SetTile(playerLoc.AsVec2(), 3);
                map->SetTile(warpPosition.AsVec2(), 3);
                map->CreatePortal(warpPosition.AsVec2(), playerLoc.AsVec2());
                warpPosition = Location();
            }
            break;
        case TK_S:
            if (terminal_state(TK_SHIFT))
            {
                RogueSaveManager::OpenWriteSaveFile("MySaveFile.rsf");
                RogueSaveManager::Write("Map", map);
                RogueSaveManager::Write("Player", playerLoc);
                RogueSaveManager::Write("Radius", radius);
                RogueSaveManager::Write("Max Pass", maxPass);
                RogueSaveManager::Write("bPoint", bresenhamPoint);
                RogueDataManager::Get()->SaveAll();
                RogueSaveManager::CloseWriteSaveFile();
                shouldBreak = true;
            }
            else
            {
                map->SetTile(playerLoc.AsVec2(), 2);
            }
            break;
        case TK_R:
            if (!terminal_state(TK_SHIFT))
            {
                map->Reset();
                map->FillTilesExc(Vec2(1, 1), Vec2(size.x - 1, size.y - 1), 1);
            }
            bresenhamPoint = Vec2(0, 0);
            break;
        }

        k = TK_G;
        terminal_clear();

        LOS::Calculate(view, playerLoc, maxPass);

        RenderMap(map, size, Vec2(x, y), playerLoc, view);
        RenderBresenham(Vec2(x, y), bresenhamPoint);
        terminal_color(color_from_argb(255, 255, 0, 255));
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
