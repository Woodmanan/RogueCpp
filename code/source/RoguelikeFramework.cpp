// RoguelikeFramework.cpp : Defines the entry point for the application.
//

#include "RoguelikeFramework.h"
#include "Core/CoreDataTypes.h"
#include "Data/RogueArena.h"
#include "Data/RogueDataManager.h"
#include "Data/SaveManager.h"
#include "Data/RegisterSaveTypes.h"
#include "Data/Resouces.h"
#include "Map/Map.h"
#include "LOS/LOS.h"
#include "LOS/TileMemory.h"
#include "Render/Windows/Window.h"
#include <format>
#include <chrono>
#include "Render/UI/Bar.h"
#include "Render/UI/Panel.h"
#include "Render/UI/Label.h"
#include "Render/UI/UIManager.h"
#include "Core/Materials/Materials.h"

using namespace std;

Color passColors[5] = {
    Color(0x17, 0xD9, 0x2A),
    Color(0x59, 0x00, 0xED),
    Color(0xFF, 0x70, 0x46),
    Color(0xD8, 0xDB, 0xDA),
    Color(0xAA, 0x55, 0xAA)
};

uint32_t render = 0x1;
uint32_t color = 0x2;
uint32_t passes = 0x4;
uint32_t heat = 0x8;
uint32_t rotations = 0x10;
uint32_t background = 0x20;

void Render_Map(THandle<Map> map, Window* window, Location playerLocation)
{
    for (int i = 0; i < window->m_rect.w; i++)
    {
        for (int j = 0; j < window->m_rect.h; j++)
        {
            Vec2 playerOffset = Vec2(i, window->m_rect.h - j) - (window->m_rect.size / 2);
            Location mapLocation = playerLocation + playerOffset;
            if (mapLocation.GetValid() && mapLocation.InMap())
            {
                THandle<BackingTile> tile = mapLocation->m_backingTile;
                window->Put(i, j, tile->m_renderCharacter,
                    Color(0x99, 0x99, 0x99),
                    tile->m_backgroundColor);
            }
        }
    }
}

void Render_View(View& view, Window* window, Location playerLocation)
{
    int maxRadius = std::min(view.GetRadius(), (int)window->m_rect.w);

    for (int i = -maxRadius; i <= maxRadius; i++)
    {
        for (int j = -maxRadius; j <= maxRadius; j++)
        {
            Vec2 windowCoord = Vec2(i, -j) + (window->m_rect.size) / 2;

            bool visible = view.GetVisibilityLocal(i, j);
            if (view.GetLocationLocal(i, j).GetValid())
            {
                if (visible)
                {
                    THandle<BackingTile> tile = view.GetLocationLocal(i,j)->m_backingTile;
                    window->Put(windowCoord.x, windowCoord.y, tile->m_renderCharacter, tile->m_foregroundColor, tile->m_backgroundColor);
                }
            }
        }
    }
}

void Render_Passes(View& view, Window* window, Location playerLocation)
{
    int maxRadius = std::min(view.GetRadius(), (int)window->m_rect.w);

    for (int i = -maxRadius; i <= maxRadius; i++)
    {
        for (int j = -maxRadius; j <= maxRadius; j++)
        {
            Vec2 windowCoord = Vec2(i, -j) + (window->m_rect.size) / 2;

            bool visible = view.GetVisibilityLocal(i, j);
            if (view.GetLocationLocal(i, j).GetValid())
            {
                if (visible)
                {
                    int step = view.GetVisibilityPassIndex(i, j) - 1;
                    Color color = passColors[step % 5];
                    window->Put(windowCoord.x, windowCoord.y, view.GetLocationLocal(i, j)->m_backingTile->m_renderCharacter, color, Color(0, 0, 0));
                }
            }
        }
    }
}

void Render_Hotspots(View& view, Window* window, Location playerLocation)
{
    int maxRadius = std::min(view.GetRadius(), (int)window->m_rect.w);

    for (int i = -maxRadius; i <= maxRadius; i++)
    {
        for (int j = -maxRadius; j <= maxRadius; j++)
        {
            Vec2 windowCoord = Vec2(i, -j) + (window->m_rect.size) / 2;

            bool visible = view.GetVisibilityLocal(i, j);
            if (visible)
            {
                Color green = Color(0x00, 0x11, 0x00);
                Color red = Color(0xFF, 0x00, 0x00);
                float heatPercent = view.Debug_GetHeatPercentageLocal(i, j);
                Color blend = Blend(green, red, heatPercent);
                Color black = Color(0, 0, 0);
                window->Put(windowCoord.x, windowCoord.y, view.GetLocationLocal(i, j)->m_backingTile->m_renderCharacter, blend, black);
            }
        }
    }
}

char rotationChars[8] = {
    '0', '1', '2', '3', '4', '5', '6', '7'
};

void Render_Rotations(View& view, Window* window, Location playerLocation)
{
    int maxRadius = std::min(view.GetRadius(), (int)window->m_rect.w);

    for (int i = -maxRadius; i <= maxRadius; i++)
    {
        for (int j = -maxRadius; j <= maxRadius; j++)
        {
            Vec2 windowCoord = Vec2(i, -j) + (window->m_rect.size) / 2;

            bool visible = view.GetVisibilityLocal(i, j);
            if (visible)
            {
                char direction = view.GetRotationLocal(i, j);
                char rotationChar = rotationChars[direction];
                Color green = Color(0x17, 0xD9, 0x2A);
                window->Put(windowCoord.x, windowCoord.y, rotationChar, green, Color(0, 0, 0));
            }
        }
    }
}

void RenderBresenham(Window* window, Vec2 endpoint)
{
    Color fg = Color(100, 0, 100);
    Color bg = Color(0, 0, 0);
    int x0 = 0;
    int y0 = 0;
    int dx = abs(endpoint.x - 0), sx = 0 < endpoint.x ? 1 : -1;
    int dy = -abs(endpoint.y - 0), sy = 0 < endpoint.y ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */

    for (;;) {  /* loop */
        Vec2 pos = (window->m_rect.size / 2) + Vec2(x0, -y0);
        window->Put(pos.x, pos.y, '*', fg, bg);
        if (x0 == endpoint.x && y0 == endpoint.y)
        {
            window->Put(pos.x, pos.y, '+', fg, bg);
            break;
        }
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
    }
}

Direction ReadDirection(Direction currentRotation)
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

    return Rotate(direction, currentRotation);
}

void ResetMap(THandle<Map> map)
{
    if (map.IsValid())
    {

    }
}

void InitManagers()
{
    MaterialManager::Get()->Init();
}

int main(int argc, char* argv[])
{
    //Initialize Random
    srand(1);

    Vec2 size(128, 64);
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

    UIManager uiManager;

    Window* gameWindow = uiManager.CreateWindow<Window>(std::string("Game window"), Anchors({ 0, 1, 0, 1 }));

    //Window gameWindow(std::string("Game window"), { 0, 1, 0, 1 });
    Panel* mainGamePanel = uiManager.CreateWindow<Panel>(std::string("Main Game Panel"), Anchors{ 0,1,0,1 }, gameWindow);
    Window* renderWindow = uiManager.CreateWindow<Window>(std::string("Render Window"), Anchors{ 0, 1, 0, 1, 1, -1, 1, -1}, mainGamePanel);
    renderWindow->m_layer = 0;

    Panel* testPanel = uiManager.CreateWindow<Panel>(std::string("Test Panel"), Anchors{ 0, .5, .8, 1 }, renderWindow);
    Panel* testPanel2 = uiManager.CreateWindow<Panel>(std::string("Test Panel 2"), Anchors{.5, 1, .8, 1, 1}, renderWindow);
    Panel* testPanel3 = uiManager.CreateWindow<Panel>(std::string("Test Panel 3"), Anchors{ 0.5, .5, 0,1, -1, 2, 1, -1 }, testPanel);

    Label* xlabel = uiManager.CreateWindow<Label>(std::string("XLabel"), Anchors{ 0,0,1,1,1,3,-3, -2 }, testPanel2);
    Bar* xbar = uiManager.CreateWindow<Bar>(std::string("XBar"), Anchors{ 0, 1, 1, 1, 3, -1, -3, -2 }, testPanel2);
    Label* ylabel = uiManager.CreateWindow<Label>(std::string("YLabel"), Anchors{ 0,0,1,1,1,3,-2, -1 }, testPanel2);
    Bar* ybar = uiManager.CreateWindow<Bar>(std::string("YBar"), Anchors{ 0, 1, 1, 1, 3, -1, -2, -1 }, testPanel2);

    Label* fpsLabel = uiManager.CreateWindow<Label>(std::string("Fps"), Anchors{ 0,1,0,0, 0,0,0,1 }, renderWindow);
    uiManager.ApplySettingsToAllWindows();

    InitManagers();

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
            map->LinkBackingTile<BackingTile>('#', Color(120, 120, 120), Color(0, 0, 0), true, -1);
            map->LinkBackingTile<BackingTile>('.', Color(100, 200, 100), Color(0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('S', Color(200, 100, 200), Color(80, 80, 80), false, 1);
            map->LinkBackingTile<BackingTile>('0', Color(60, 60, 255), Color(0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('1', Color(60, 60, 255), Color(0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('2', Color(60, 60, 255), Color(0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('3', Color(60, 60, 255), Color(0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('4', Color(60, 60, 255), Color(0, 0, 0), false, 1);
            map->LinkBackingTile<BackingTile>('5', Color(60, 60, 255), Color(0, 0, 0), false, 1);

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
    char buffer[50];
    wchar_t wbuffer[50];

    terminal_open();
    std::snprintf(&buffer[0], 50, "window: size=%dx%d", (x + 1), (y + 1));
    terminal_set(&buffer[0]);
    terminal_set("input: filter = [keyboard, mouse+]");
    terminal_set("window.resizeable=true");
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
                RogueSaveManager::DeleteSaveFile("UISettings.rsf");
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
                    Direction direction = ReadDirection(lookDirection);
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
                warpDirection = ReadDirection(lookDirection);
            }
            else
            {
                //Establish the warp!
                currentIndex = std::min(currentIndex + 1, 5);
                map->SetTile(playerLoc.AsVec2(), currentIndex + 3);
                map->SetTile(warpPosition.AsVec2(), currentIndex + 3);

                Direction exitDirection = ReadDirection(lookDirection);

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
                renderFlags |= rotations;
            }
            else if (renderFlags & rotations)
            {
                renderFlags ^= rotations;
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
        case TK_PAGEUP:
            x++;
            y++;
            std::snprintf(&buffer[0], 50, "window: size=%dx%d", (x + 1), (y + 1));
            terminal_set(&buffer[0]);
            break;
        case TK_PAGEDOWN:
            x--;
            y--;
            std::snprintf(&buffer[0], 50, "window: size=%dx%d", (x + 1), (y + 1));
            terminal_set(&buffer[0]);
            break;
        case TK_A: //Anchors!
            Window* selected = nullptr;

            terminal_clear();
            gameWindow->RenderSelection();
            Window* hovered = gameWindow->GetSelectedWindow();
            if (hovered != nullptr)
            {
                hovered->RenderAnchors();
            }
            terminal_refresh();

            while (selected == nullptr)
            {
                if (terminal_has_input())
                {
                    switch (terminal_read())
                    {
                        case TK_MOUSE_MOVE:
                        {
                            terminal_clear();
                            gameWindow->UpdateLayout({ 0, 0, (short)terminal_state(TK_WIDTH), (short)terminal_state(TK_HEIGHT) });
                            gameWindow->RenderSelection();
                            Window* hovered = gameWindow->GetSelectedWindow();
                            if (hovered != nullptr)
                            {
                                hovered->RenderAnchors();
                            }
                            terminal_refresh();
                            break;
                        }
                        case TK_MOUSE_LEFT:
                            selected = gameWindow->GetSelectedWindow();
                            break;
                    }
                }
            }

            bool moving = true;
            while (moving)
            {
                terminal_clear();
                gameWindow->UpdateLayout({ 0, 0, (short)terminal_state(TK_WIDTH), (short)terminal_state(TK_HEIGHT) });
                gameWindow->RenderContent(0);
                terminal_refresh();

                if (terminal_has_input())
                {
                    if (terminal_state(TK_SHIFT))
                    {
                        switch (terminal_peek())
                        {
                            case TK_UP:
                                selected->m_anchors.minYOffset--;
                                break;
                            case TK_DOWN:
                                selected->m_anchors.maxYOffset++;
                                break;
                            case TK_LEFT:
                                selected->m_anchors.minXOffset--;
                                break;
                            case TK_RIGHT:
                                selected->m_anchors.maxXOffset++;
                                break;
                        }
                    }
                    else if (terminal_state(TK_CONTROL))
                    {
                        switch (terminal_peek())
                        {
                        case TK_UP:
                            selected->m_anchors.maxYOffset--;
                            break;
                        case TK_DOWN:
                            selected->m_anchors.minYOffset++;
                            break;
                        case TK_LEFT:
                            selected->m_anchors.maxXOffset--;
                            break;
                        case TK_RIGHT:
                            selected->m_anchors.minXOffset++;
                            break;
                        }
                    }
                    else
                    {
                        switch (terminal_peek())
                        {
                            case TK_UP:
                                selected->m_anchors.minYOffset--;
                                selected->m_anchors.maxYOffset--;
                                break;
                            case TK_DOWN:
                                selected->m_anchors.minYOffset++;
                                selected->m_anchors.maxYOffset++;
                                break;
                            case TK_LEFT:
                                selected->m_anchors.minXOffset--;
                                selected->m_anchors.maxXOffset--;
                                break;
                            case TK_RIGHT:
                                selected->m_anchors.minXOffset++;
                                selected->m_anchors.maxXOffset++;
                                break;
                        }
                    }

                    switch (terminal_read())
                    {
                    case TK_X: {
                        char buf1[50];
                        std::snprintf(buf1, 50, "");
                        terminal_read_str(1, 1, &buf1[0], 50);
                        float min = std::stof(std::string(buf1));
                        std::snprintf(buf1, 50, "");
                        terminal_read_str(1, 1, &buf1[0], 50);
                        float max = std::stof(std::string(buf1));
                        selected->m_anchors.minX = min;
                        selected->m_anchors.maxX = max;
                        selected->m_anchors.minXOffset = 0;
                        selected->m_anchors.maxXOffset = 0;
                        break;
                    }
                    case TK_Y: {
                        char buf1[50];
                        std::snprintf(buf1, 50, "");
                        terminal_read_str(1, 1, &buf1[0], 50);
                        float min = std::stof(std::string(buf1));
                        std::snprintf(buf1, 50, "");
                        terminal_read_str(1, 1, &buf1[0], 50);
                        float max = std::stof(std::string(buf1));
                        selected->m_anchors.minY = min;
                        selected->m_anchors.maxY = max;
                        selected->m_anchors.minYOffset = 0;
                        selected->m_anchors.maxYOffset = 0;
                        break;
                    }
                    case TK_Q:
                        moving = false;
                        uiManager.CreateSettingsForAllWindows();
                        uiManager.SaveSettings();
                        break;
                    case TK_P:
                        while (true)
                        {
                            terminal_clear();
                            gameWindow->UpdateLayout({ 0, 0, (short)terminal_state(TK_WIDTH), (short)terminal_state(TK_HEIGHT) });
                            gameWindow->RenderSelection();
                            Window* hovered = gameWindow->GetSelectedWindow();
                            if (hovered != nullptr)
                            {
                                hovered->RenderAnchors();
                            }
                            terminal_refresh();

                            if (terminal_has_input() && terminal_read() == TK_MOUSE_LEFT)
                            {
                                selected->SetParent(hovered);
                                break;
                            }
                        }
                        break;
                    }
                }

            }
        }

        k = TK_G;

        terminal_clear();

        auto currentTime = chrono::system_clock::now();
        auto frameTime = chrono::duration_cast<chrono::duration<float>>(currentTime - clock);

        //Update UI Layout
        gameWindow->UpdateLayout({ 0, 0, (short)terminal_state(TK_WIDTH), (short)terminal_state(TK_HEIGHT) });

        LOS::Calculate(view, playerLoc, lookDirection, maxPass);
        memory->Update(view);

        if (renderFlags & render)
        {
            if (renderFlags & background)
            {
                memory->Render(renderWindow);
            }

            Render_View(view, renderWindow, playerLoc);
            if (renderFlags & color)
            {
                Render_Passes(view, renderWindow, playerLoc);
            }
            if (renderFlags & heat)
            {
                Render_Hotspots(view, renderWindow, playerLoc);
                std::snprintf(&buffer[0], 15, "Max Heat: %d", view.Debug_GetMaxHeat());
                terminal_color(Color(255, 0x55, 0x55));
                terminal_print(0, 1, &buffer[0]);

                std::snprintf(&buffer[0], 20, "Sum Heat: %d", view.Debug_GetSumHeat());
                terminal_print(15, 1, &buffer[0]);

                int maxTiles = view.Debug_GetNumRevealed();

                std::snprintf(&buffer[0], 30, "NumTiles: %d (%.0f%%)", maxTiles, ((float) view.Debug_GetSumHeat() * 100) / maxTiles);
                terminal_print(35, 1, &buffer[0]);

            }
            if (renderFlags & rotations)
            {
                Render_Rotations(view, renderWindow, playerLoc);
            }
            RenderBresenham(renderWindow, bresenhamPoint);

            renderWindow->Put(renderWindow->m_rect.w / 2, renderWindow->m_rect.h / 2, '@', Color(255, 0, 255), Color(0, 0, 0));
        }


        xbar->SetColor(Color(255, 100, 0));
        xbar->SetFill(((float)playerLoc.x()) / (map->m_size.x - 1), 0.03f);
        xlabel->SetString("x:");
        xlabel->SetAlignment(TK_ALIGN_DEFAULT);

        ybar->SetColor(Color(255, 100, 0));
        ybar->SetFill(((float)playerLoc.y()) / (map->m_size.y - 1), 0.03f);
        ylabel->SetString("y:");
        ylabel->SetAlignment(TK_ALIGN_DEFAULT);
        gameWindow->RenderContent(frameTime.count());

        std::snprintf(&buffer[0], 50, "FPS: %.1f (%d x %d) (%d passes)", FPS, view.GetRadius(), view.GetRadius(), maxPass);
        terminal_color(Color(255, 0, 255));
        fpsLabel->SetAlignment(TK_ALIGN_DEFAULT);
        fpsLabel->SetString(buffer);

        terminal_refresh();
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
