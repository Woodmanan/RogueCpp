// RoguelikeFramework.cpp : Defines the entry point for the application.
//

#include "RoguelikeFramework.h"
#include "Core/CoreDataTypes.h"
#include "Data/RogueArena.h"
#include "Data/RogueDataManager.h"
#include "Data/SaveManager.h"
#include "Data/RegisterSaveTypes.h"
#include "Map/Map.h"
#include "LOS/LOS.h"
#include "LOS/TileMemory.h"
#include <format>
#include <chrono>

using namespace std;

color_t passColors[5] = {
    color_from_argb(0xFF, 0x17, 0xD9, 0x2A),
    color_from_argb(0xFF, 0x59, 0x00, 0xED),
    color_from_argb(0xFF, 0xFF, 0x70, 0x46),
    color_from_argb(0xFF, 0xD8, 0xDB, 0xDA),
    color_from_argb(0xFF, 0xAA, 0x55, 0xAA)
};

color_t Blend(color_t a, color_t b)
{
    uint32_t alpha = (((a >> 24) & 0xFF) + ((b >> 24) & 0xFF)) / 2 & 0xFF;
    uint32_t red =   (((a >> 16) & 0xFF) + ((b >> 16) & 0xFF)) / 2 & 0xFF;
    uint32_t green = (((a >>  8) & 0xFF) + ((b >>  8) & 0xFF)) / 2 & 0xFF;
    uint32_t blue =  (((a >>  0) & 0xFF) + ((b >>  0) & 0xFF)) / 2 & 0xFF;
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

uint32_t render = 0x1;
uint32_t color = 0x2;
uint32_t passes = 0x4;
uint32_t heat = 0x8;
uint32_t background = 0x10;

void Render_Map(THandle<Map> map, Vec2 window, Location playerLocation)
{
    for (int i = 0; i < window.x; i++)
    {
        for (int j = 0; j < window.y; j++)
        {
            Vec2 playerOffset = Vec2(i, window.y - j) - Vec2(40, 20);
            Location mapLocation = playerLocation + playerOffset;
            if (mapLocation.GetValid() && mapLocation.InMap())
            {
                terminal_color(color_from_argb(0xFF, 0x99, 0x99, 0x99));
                terminal_bkcolor(mapLocation->m_backingTile->m_backgroundColor);
                terminal_put(i, j, mapLocation->m_backingTile->m_renderCharacter);
            }
        }
    }
}

void Render_View(View& view, Vec2 window, Location playerLocation)
{
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
                    terminal_color(view.GetLocationLocal(i, j)->m_backingTile->m_foregroundColor);
                    terminal_bkcolor(color_from_argb(255, 0, 0, 0));
                    terminal_put(windowCoord.x, windowCoord.y, view.GetLocationLocal(i, j)->m_backingTile->m_renderCharacter);
                }
            }
        }
    }
}

void Render_Passes(View& view, Vec2 window, Location playerLocation)
{
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
                    int step = view.GetVisibilityPassIndex(i, j) - 1;
                    color_t color = passColors[step % 5];
                    terminal_color(color);
                    terminal_bkcolor(color_from_argb(255, 0, 0, 0));
                    terminal_put(windowCoord.x, windowCoord.y, view.GetLocationLocal(i, j)->m_backingTile->m_renderCharacter);
                }
            }
        }
    }
}

void Render_Hotspots(View& view, Vec2 window, Location playerLocation)
{
    int maxRadius = std::min(view.GetRadius(), (int)window.x);

    for (int i = -maxRadius; i <= maxRadius; i++)
    {
        for (int j = -maxRadius; j <= maxRadius; j++)
        {
            Vec2 windowCoord = Vec2(i, -j) + Vec2(40, 20);

            bool visible = view.GetVisibilityLocal(i, j);
            if (visible)
            {
                color_t green = color_from_argb(0xFF, 0x00, 0x11, 0x00);
                color_t red = color_from_argb(0xFF, 0xFF, 0x00, 0x00);
                float heatPercent = view.Debug_GetHeatPercentageLocal(i, j);
                terminal_color(Blend(green, red, heatPercent));
                terminal_put(windowCoord.x, windowCoord.y, view.GetLocationLocal(i, j)->m_backingTile->m_renderCharacter);
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

Direction ReadDirection()
{
    Direction direction = North;

    switch (terminal_read())
    {
    case TK_K:
    case TK_UP:
        direction = North;
        break;
    case TK_U:
        direction = NorthEast;
        break;
    case TK_L:
    case TK_RIGHT:
        direction = East;
        break;
    case TK_N:
        direction = SouthEast;
        break;
    case TK_J:
    case TK_DOWN:
        direction = South;
        break;
    case TK_B:
        direction = SouthWest;
        break;
    case TK_H:
    case TK_LEFT:
        direction = West;
        break;
    case TK_Y:
        direction = NorthWest;
        break;
    }

    return direction;
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

    Vec2 size(64, 64);
    THandle<Map> map;
    THandle<TileMemory> memory;
    Location playerLoc = Location(1, 1, 0);
    Direction lookDirection = North;
    Location warpPosition;
    Direction warpDirection;
    int radius = 10;
    int maxPass = 10;
    int currentIndex = -1;
    Vec2 bresenhamPoint = Vec2(0, 0);
    uint32_t renderFlags = (render | background);

    const int fpsMerge = 60;
    int fpsIndex = 0;
    float FPSBuffer[fpsMerge];

    if (RogueSaveManager::FileExists("MySaveFile.rsf"))
    {
        RogueSaveManager::OpenReadSaveFile("MySaveFile.rsf");
        RogueSaveManager::Read("Map", map);
        RogueSaveManager::Read("Memory", memory);
        RogueSaveManager::Read("Player", playerLoc);
        RogueSaveManager::Read("Look Direction", lookDirection);
        RogueSaveManager::Read("Radius", radius);
        RogueSaveManager::Read("Max Pass", maxPass);
        RogueSaveManager::Read("Current Index", currentIndex);
        RogueSaveManager::Read("Render Flags", renderFlags);
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
            map->LinkBackingTile<BackingTile>('.', color_from_argb(255, 100, 200, 100), color_from_argb(255, 0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('S', color_from_argb(255, 200, 100, 200), color_from_argb(255, 80, 80, 80), false, 1);
            map->LinkBackingTile<BackingTile>('0', color_from_argb(255, 60, 60, 255), color_from_argb(255, 0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('1', color_from_argb(255, 60, 60, 255), color_from_argb(255, 0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('2', color_from_argb(255, 60, 60, 255), color_from_argb(255, 0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('3', color_from_argb(255, 60, 60, 255), color_from_argb(255, 0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('4', color_from_argb(255, 60, 60, 255), color_from_argb(255, 0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('5', color_from_argb(255, 60, 60, 255), color_from_argb(255, 0, 0, 0), false, 1);

            map->FillTilesExc(Vec2(0, 0), size, 0);
            map->FillTilesExc(Vec2(1, 1), Vec2(size.x - 1, size.y - 1), 1);
            
            memory = RogueDataManager::Allocate<TileMemory>(map);
            memory->SetLocalPosition(playerLoc);
        }
    }

    char k;
    int x = 80, y = 40;
    int pos_x = 40;
    int pos_y = 20;
    terminal_open();
    terminal_set("window: size=81x41");
    terminal_print(34, 20, "Hello World! I am here!");
    //terminal_print(34, 21, std::to_string(test.GetID()).c_str());
    terminal_refresh();

    View view;
    view.SetRadius(radius);

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
                auto move = playerLoc.Traverse(Direction::West, lookDirection);
                playerLoc = move.first;
                lookDirection = Rotate(lookDirection, move.second);
                memory->Move(VectorFromDirection(Direction::West));
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
                auto move = playerLoc.Traverse(Direction::North, lookDirection);
                playerLoc = move.first;
                lookDirection = Rotate(lookDirection, move.second);
                memory->Move(VectorFromDirection(Direction::North));
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
                auto move = playerLoc.Traverse(Direction::South, lookDirection);
                playerLoc = move.first;
                lookDirection = Rotate(lookDirection, move.second);
                memory->Move(VectorFromDirection(Direction::South));
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
                auto move = playerLoc.Traverse(Direction::East, lookDirection);
                playerLoc = move.first;
                lookDirection = Rotate(lookDirection, move.second);
                memory->Move(VectorFromDirection(Direction::East));
            }
            break;
        case TK_Y:
            if (terminal_state(TK_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(-1, 1);
            }
            else
            {
                auto move = playerLoc.Traverse(Direction::NorthWest, lookDirection);
                playerLoc = move.first;
                lookDirection = Rotate(lookDirection, move.second);
                memory->Move(VectorFromDirection(Direction::NorthWest));
            }
            break;
        case TK_U:
            if (terminal_state(TK_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(1, 1);
            }
            else
            {
                auto move = playerLoc.Traverse(Direction::NorthEast, lookDirection);
                playerLoc = move.first;
                lookDirection = Rotate(lookDirection, move.second);
                memory->Move(VectorFromDirection(Direction::NorthEast));
            }
            break;
        case TK_B:
            if (terminal_state(TK_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(-1, -1);
            }
            else
            {
                auto move = playerLoc.Traverse(Direction::SouthWest, lookDirection);
                playerLoc = move.first;
                lookDirection = Rotate(lookDirection, move.second);
                memory->Move(VectorFromDirection(Direction::SouthWest));
            }
            break;
        case TK_N:
            if (terminal_state(TK_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(1, -1);
            }
            else
            {
                auto move = playerLoc.Traverse(Direction::SouthEast, lookDirection);
                playerLoc = move.first;
                lookDirection = Rotate(lookDirection, move.second);
                memory->Move(VectorFromDirection(Direction::SouthEast));
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
                currentIndex = std::min(currentIndex + 1, 5);
                map->SetTile(playerLoc.AsVec2(), currentIndex + 3);
                map->SetTile(warpPosition.AsVec2(), currentIndex + 3);

                if (terminal_state(TK_SHIFT))
                {
                    Direction direction = ReadDirection();
                    map->CreateDirectionalPortal(warpPosition.AsVec2(), playerLoc.AsVec2(), direction);
                }
                else
                {
                    map->CreatePortal(warpPosition.AsVec2(), playerLoc.AsVec2());
                }
                warpPosition = Location();
            }
            break;
        case TK_D:
            if (!warpPosition.GetValid())
            {
                warpPosition = playerLoc;
                warpDirection = ReadDirection();
            }
            else
            {
                //Establish the warp!
                currentIndex = std::min(currentIndex + 1, 5);
                map->SetTile(playerLoc.AsVec2(), currentIndex + 3);
                map->SetTile(warpPosition.AsVec2(), currentIndex + 3);

                Direction exitDirection = ReadDirection();

                map->CreateBidirectionalPortal(warpPosition.AsVec2(), warpDirection, playerLoc.AsVec2(), exitDirection);

                //Reset
                warpPosition = Location();
            }
            break;
        case TK_S:
            if (terminal_state(TK_SHIFT))
            {
                RogueSaveManager::OpenWriteSaveFile("MySaveFile.rsf");
                RogueSaveManager::Write("Map", map);
                RogueSaveManager::Write("Memory", memory);
                RogueSaveManager::Write("Player", playerLoc);
                RogueSaveManager::Write("Look Direction", lookDirection);
                RogueSaveManager::Write("Radius", radius);
                RogueSaveManager::Write("Max Pass", maxPass);
                RogueSaveManager::Write("Current Index", currentIndex);
                RogueSaveManager::Write("Render Flags", renderFlags);
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
                map->FillTilesExc(Vec2(0, 0), Vec2(size.x - 0, size.y - 0), 1);
                memory->Wipe();
            }
            bresenhamPoint = Vec2(0, 0);
            break;
        case TK_C:
            if (renderFlags & color)
            {
                renderFlags ^= color;
                renderFlags |= heat;
            }
            else if (renderFlags & heat)
            {
                renderFlags ^= heat;
                renderFlags ^= render;
            }
            else if (~renderFlags & render)
            {
                renderFlags |= render;
            }
            else
            {
                renderFlags |= color;
            }
            break;
        case TK_X:
            renderFlags ^= background;
            break;
        case TK_9:
            lookDirection = Rotate(lookDirection, West);
            break;
        case TK_0:
            lookDirection = Rotate(lookDirection, East);
            break;
        case TK_W:
            memory->Wipe();
            break;
        }

        k = TK_G;
        clock = chrono::system_clock::now();
        char buffer[30];

        terminal_clear();
        LOS::Calculate(view, playerLoc, lookDirection, maxPass);
        memory->Update(view);

        if (renderFlags & render)
        {
            if (renderFlags & background)
            {
                memory->Render(Vec2(x, y));
            }

            Render_View(view, Vec2(x, y), playerLoc);
            if (renderFlags & color)
            {
                Render_Passes(view, Vec2(x, y), playerLoc);
            }
            if (renderFlags & heat)
            {
                Render_Hotspots(view, Vec2(x, y), playerLoc);
                std::snprintf(&buffer[0], 15, "Max Heat: %d", view.Debug_GetMaxHeat());
                terminal_color(color_from_argb(255, 255, 0x55, 0x55));
                terminal_print(0, 1, &buffer[0]);

                std::snprintf(&buffer[0], 20, "Sum Heat: %d", view.Debug_GetSumHeat());
                terminal_print(15, 1, &buffer[0]);

                int maxTiles = view.Debug_GetNumRevealed();

                std::snprintf(&buffer[0], 30, "NumTiles: %d (%.0f%%)", maxTiles, ((float) view.Debug_GetSumHeat() * 100) / maxTiles);
                terminal_print(35, 1, &buffer[0]);

            }
            RenderBresenham(Vec2(x, y), bresenhamPoint);
            terminal_color(color_from_argb(255, 255, 0, 255));
            terminal_put(40, 20, '@');
        }

        std::snprintf(&buffer[0], 15, "FPS: %.1f", FPS);
        terminal_print(0, 0, &buffer[0]);
        terminal_refresh();
        auto currentTime = chrono::system_clock::now();
        auto frameTime = chrono::duration_cast<chrono::duration<float>>(currentTime - clock);
        clock = currentTime;

        FPSBuffer[fpsIndex] = 1.0f / frameTime.count();
        fpsIndex = (fpsIndex + 1) % fpsMerge;

        FPS = 0.0f;
        for (int i = 0; i < fpsMerge; i++)
        {
            FPS += FPSBuffer[i];
        }

        FPS = FPS / fpsMerge;
    }
    terminal_close();

    return EXIT_SUCCESS;
}
