// RoguelikeFramework.cpp : Defines the entry point for the application.
//

#include "RoguelikeFramework.h"
#include "Core/CoreDataTypes.h"
#include "Data/RogueArena.h"
#include "Data/RogueDataManager.h"
#include "Data/JobSystem.h"
#include "Data/SaveManager.h"
#include "Data/RegisterSaveTypes.h"
#include "Data/Resources.h"
#include "Core/Stats/StatManager.h"
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
#include "Core/Events/Event.h"
#include "Debug/Profiling.h"
#include "Render/Fonts/FontManager.h"
#include "Render/Terminal.h"
#include "Game/Game.h"
#include "Utils/Utils.h"

#include "Data/Serialization/BitStream.h"
#include "Data/Serialization/Serialization.h"

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

void Render_Player_Data(PlayerData& data, Window* window, Location playerLocation)
{
    ROGUE_PROFILE;

    View& view = data.GetCurrentView();
    TileMemory& memory = data.GetCurrentMemory();

    for (int i = 0; i < window->m_rect.w; i++)
    {
        for (int j = 0; j < window->m_rect.h; j++)
        {
            int localX = i - window->m_rect.w / 2;
            int localY = -j + window->m_rect.h / 2;

            if (memory.ValidTile(localX, localY))
            {
                DataTile& dataTile = data.GetTileForLocal(localX, localY);
                BackingTile& tile = data.GetBackingTileForLocal(localX, localY);

                bool visible = (std::abs(localX) <= view.GetRadius() && std::abs(localY) <= view.GetRadius() && view.GetVisibilityLocal(localX, localY));

                if (visible)
                {
                    window->Put(i, j, dataTile.m_renderChar, dataTile.m_color, dataTile.m_color);
                }
                else
                {
                    Color greyish = Color(10, 10, 10);
                    Color fgBlend = Blend(dataTile.m_color, greyish, 0.5f);
                    Color bgBlend = Blend(dataTile.m_color, greyish, 0.5f);
                    window->Put(i, j, dataTile.m_renderChar, fgBlend, bgBlend);
                }
            }
            else
            {
                window->Put(i, j, '2', Color(255, 0, 0), Color(0, 0, 0));
            }
        }
    }

    //Temp! Render character character
    window->Put(window->m_rect.w / 2, window->m_rect.h / 2, '@', Color(0,0,0), Color(255,255,255));
}

void Render_Temperature(PlayerData& data, Window* window, Location playerLocation)
{
    ROGUE_PROFILE;

    View& view = data.GetCurrentView();
    TileMemory& memory = data.GetCurrentMemory();

    for (int i = 0; i < window->m_rect.w; i++)
    {
        for (int j = 0; j < window->m_rect.h; j++)
        {
            int localX = i - window->m_rect.w / 2;
            int localY = -j + window->m_rect.h / 2;

            if (memory.ValidTile(localX, localY))
            {
                DataTile& tile = data.GetTileForLocal(localX, localY);
                BackingTile& backing = data.GetBackingTileForLocal(localX, localY);

                bool visible = (std::abs(localX) <= view.GetRadius() && std::abs(localY) <= view.GetRadius() && view.GetVisibilityLocal(localX, localY));

                Color cold = Color(0, 0, 70);
                Color hot = Color(255, 0, 0);
                Color fgBlend = tile.m_color;
                Color bgBlend = Blend(cold, hot, std::clamp(((float)tile.m_temperature / 4.0f), 0.0f, 1.0f));

                if (visible)
                {
                    window->Put(i, j, tile.m_renderChar, bgBlend, bgBlend);
                }
                else
                {
                    window->Put(i, j, tile.m_renderChar, Blend(bgBlend, Color(50, 50, 50), .8f), bgBlend);
                }
            }
            else
            {
                window->Put(i, j, '2', Color(255, 0, 0), Color(0, 0, 0));
            }
        }
    }

    //Temp! Render character character
    window->Put(window->m_rect.w / 2, window->m_rect.h / 2, '@', Color(0, 0, 0), Color(255, 255, 255));
}

void RenderBresenham(Window* window, Vec2 endpoint)
{
    ROGUE_PROFILE;
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
    case EKey::K:
    case EKey::UP:
        direction = North;
        break;
    case EKey::U:
        direction = NorthEast;
        break;
    case EKey::L:
    case EKey::RIGHT:
        direction = East;
        break;
    case EKey::N:
        direction = SouthEast;
        break;
    case EKey::J:
    case EKey::DOWN:
        direction = South;
        break;
    case EKey::B:
        direction = SouthWest;
        break;
    case EKey::H:
    case EKey::LEFT:
        direction = West;
        break;
    case EKey::Y:
        direction = NorthWest;
        break;
    }

    return Rotate(direction, currentRotation);
}

void SetupResources(uint resourceThreads)
{
    ResourceManager::InitResources();

    //TODO: Add new pieces!
    ResourceManager::Get()->Register("Mat", &RogueResources::PackMaterial, &RogueResources::LoadMaterial);
    ResourceManager::Get()->Register("Reaction", &RogueResources::PackReaction, &RogueResources::LoadReaction);
    ResourceManager::Get()->Register("Image", &RogueResources::PackImage, &RogueResources::LoadImage);
    ResourceManager::Get()->Register("Stats", &RogueResources::PackStatDefinition, RogueResources::LoadStatDefinition);

    auto PackFont = GetMember(FontManager::Get(), &FontManager::PackFont);
    auto LoadFont = GetMember(FontManager::Get(), &FontManager::LoadFont);
    ResourceManager::Get()->Register("Font", PackFont, LoadFont);

    ResourceManager::Get()->LaunchThreads(resourceThreads);
}

void TestEvents()
{
    ROGUE_PROFILE_SECTION("Test Events");
    EventHandlerContainer testContainer1;
    EventHandlerContainer testContainer2;
    EventHandlerContainer testContainer3;

    for (int i = 0; i < 30; i++)
    {
        testContainer1.eventHandlers.push_back(new TestEvent<OnTurnStartGlobal, 10, 0>());
        testContainer2.eventHandlers.push_back(new TestEvent<OnTurnStartGlobal, 20, 1>());
        testContainer3.eventHandlers.push_back(new TestEvent<OnTurnStartGlobal, 30, 2>());

        testContainer1.eventHandlers.push_back(new TestEvent<OnTurnEndGlobal, 30, 0>());
        testContainer2.eventHandlers.push_back(new TestEvent<OnTurnEndGlobal, 20, 1>());
        testContainer3.eventHandlers.push_back(new TestEvent<OnTurnEndGlobal, 10, 2>());
    }

    for (int i = 0; i < 100; i++)
    {
        EventData data;
        data.targets.push_back(&testContainer1);
        data.targets.push_back(&testContainer2);
        data.targets.push_back(&testContainer3);

        {
            ROGUE_PROFILE_SECTION("Test Event: 0");
            data.type = OnTurnStartGlobal;
            FireEvent(data);
        }

        {
            ROGUE_PROFILE_SECTION("Test Event: 1");
            data.type = OnTurnEndGlobal;
            FireEvent(data);
        }

        {
            ROGUE_PROFILE_SECTION("Test Event: 2");
            data.type = OnAttacked;
            FireEvent(data);
        }
    }
}

int main(int argc, char* argv[])
{
    uint maxThreads = std::thread::hardware_concurrency();
    DEBUG_PRINT("%d concurrent threads are supported.", maxThreads);

    if (maxThreads == 0)
    {
        DEBUG_PRINT("Fallback - assuming there are 8 threads.");
        maxThreads == 8; //Fallback - assume 8 threads available at the minimum
    }

    uint reservedThreads = 3;  //1 thread for main, 1 for resource manager, 1 for game thread
    maxThreads -= reservedThreads;

    uint numResourceThreads = (maxThreads * 4) / 10;
    uint numJobThreads = maxThreads - numResourceThreads;

    Jobs::Initialize(numJobThreads);
    SetupResources(numResourceThreads);

    //Initialize Random
    srand(1);

    Game game = Game();
    std::thread gameThread(&Game::LaunchGame, &game);
    bool gameReady = false;
    EDrawState drawState = EDrawState::Normal;

    if (RogueSaveManager::FilePathExists("TestSave.rsf"))
    {
        game.CreateInput<LoadSaveGame>("TestSave.rsf");
    }
    else
    {
        game.CreateInput<BeginSeededGame>(420);
    }

    PlayerData playerData;

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

    UIManager uiManager;

    Window* gameWindow = uiManager.CreateWindow<Window>(std::string("Game window"), Anchors({ 0, 1, 0, 1 }));

    //Window gameWindow(std::string("Game window"), { 0, 1, 0, 1 });
    Panel* mainGamePanel = uiManager.CreateWindow<Panel>(std::string("Main Game Panel"), Anchors{ 0,1,0,1 }, gameWindow);
    Window* renderWindow = uiManager.CreateWindow<Window>(std::string("Render Window"), Anchors{ 0, 1, 0, 1, 1, -1, 1, -1}, mainGamePanel);

    Panel* testPanel = uiManager.CreateWindow<Panel>(std::string("Test Panel"), Anchors{ 0, .5, .8, 1 }, renderWindow);
    Panel* testPanel2 = uiManager.CreateWindow<Panel>(std::string("Test Panel 2"), Anchors{.5, 1, .8, 1, 1}, renderWindow);
    Panel* testPanel3 = uiManager.CreateWindow<Panel>(std::string("Test Panel 3"), Anchors{ 0.5, .5, 0,1, -1, 2, 1, -1 }, testPanel);

    Label* xlabel = uiManager.CreateWindow<Label>(std::string("XLabel"), Anchors{ 0,0,1,1,1,3,-3, -2 }, testPanel2);
    Bar* xbar = uiManager.CreateWindow<Bar>(std::string("XBar"), Anchors{ 0, 1, 1, 1, 3, -1, -3, -2 }, testPanel2);
    Label* ylabel = uiManager.CreateWindow<Label>(std::string("YLabel"), Anchors{ 0,0,1,1,1,3,-2, -1 }, testPanel2);
    Bar* ybar = uiManager.CreateWindow<Bar>(std::string("YBar"), Anchors{ 0, 1, 1, 1, 3, -1, -2, -1 }, testPanel2);

    uiManager.ApplySettingsToAllWindows();

    int k;
    int x = 80, y = 40;
    int pos_x = 40;
    int pos_y = 20;
    char buffer[50];
    wchar_t wbuffer[50];

    terminal_custom_init();
    terminal_print(34, 20, "Hello World! I am here!");
    terminal_refresh();

    TestEvents();

    View view;
    view.SetRadius(radius);

    auto clock = chrono::system_clock::now();

    bool shouldBreak = false;
    long frame = 0;
    while (!shouldBreak && !terminal_should_close()) {
        terminal_update();

        if (terminal_has_input())
        {
            k = terminal_read();
        }
        switch (k)
        {
        case EKey::Q:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                RogueSaveManager::DeleteSaveFile("TestSave.rsf");
                RogueSaveManager::DeleteSaveFile("UISettings.rsf");
                game.CreateInput<ExitGame>();
            }
            shouldBreak = true;
            break;
        case EKey::H:
        case EKey::LEFT:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(-1, 0);
            }
            else
            {
                game.CreateInput<Movement>(Direction::West);
            }
            break;
        case EKey::K:
        case EKey::UP:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(0, 1);
            }
            else
            {
                game.CreateInput<Movement>(Direction::North);
            }
            break;
        case EKey::J:
        case EKey::DOWN:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(0, -1);
            }
            else
            {
                game.CreateInput<Movement>(Direction::South);
                //auto move = playerLoc.Traverse(Direction::South, lookDirection);
                //playerLoc = move.first;
                //lookDirection = Rotate(lookDirection, move.second);
                //memory->Move(VectorFromDirection(Direction::South));
            }
            break;
        case EKey::L:
        case EKey::RIGHT:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(1, 0);
            }
            else
            {
                game.CreateInput<Movement>(Direction::East);
            }
            break;
        case EKey::Y:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(-1, 1);
            }
            else
            {
                game.CreateInput<Movement>(Direction::NorthWest);
            }
            break;
        case EKey::U:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(1, 1);
            }
            else
            {
                game.CreateInput<Movement>(Direction::NorthEast);
            }
            break;
        case EKey::B:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(-1, -1);
            }
            else
            {
                game.CreateInput<Movement>(Direction::SouthWest);
            }
            break;
        case EKey::N:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                bresenhamPoint = bresenhamPoint + Vec2(1, -1);
            }
            else
            {
                game.CreateInput<Movement>(Direction::SouthEast);
            }
            break;
        case EKey::F:
            game.CreateInput<DEBUG_FIRE>();
            break;
        case EKey::SPACE:
            game.CreateInput<Wait>();
            //map->SetTile(playerLoc.AsVec2(), !playerLoc->m_backingTile->m_index);
            break;
        case EKey::TAB:
            CycleDrawState(drawState);
            break;
        case EKey::EQUAL:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                maxPass = maxPass + 1;
            }
            else
            {
                radius = radius + 1;
                view.SetRadius(radius);
            }
            break;
        case EKey::MINUS:
            if (terminal_get_key(EKey::LEFT_SHIFT))
            {
                maxPass = std::max(1, maxPass - 1);
            }
            else
            {
                radius = std::max(0, radius - 1);
                view.SetRadius(radius);
            }
            break;
        case EKey::S:
            game.CreateInput<SaveAndExit>();
            shouldBreak = true;
            break;
        case EKey::R:
        {
            /*if (!terminal_get_key(EKey::LEFT_SHIFT))
            {
                map->Reset();
                map->FillTilesExc(Vec2(0, 0), Vec2(size.x - 0, size.y - 0), 1);
                memory->Wipe();
            }
            bresenhamPoint = Vec2(0, 0);*/
            game.CreateInput<DEBUG_LOAD_RESOURCES>();
            break;
        }
        case EKey::C:
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
        case EKey::X:
            renderFlags ^= background;
            break;
        case EKey::NUM_9:
            lookDirection = Rotate(lookDirection, West);
            break;
        case EKey::NUM_0:
            lookDirection = Rotate(lookDirection, East);
            break;
        case EKey::W:
            game.CreateInput<DEBUG_MAKE_STONE>();
            break;
        case EKey::PAGE_UP:
            //TODO: RESIZING
            x++;
            y++;
            std::snprintf(&buffer[0], 50, "window: size=%dx%d", (x + 1), (y + 1));
            terminal_set(&buffer[0]);
            break;
        case EKey::PAGE_DOWN:
            //TODO: RESIZING
            x--;
            y--;
            std::snprintf(&buffer[0], 50, "window: size=%dx%d", (x + 1), (y + 1));
            terminal_set(&buffer[0]);
            break;
        case EKey::A: //Anchors!
            Window* selected = nullptr;

            terminal_clear();
            gameWindow->RenderSelection();
            Window* hovered = gameWindow->GetSelectedWindow();
            if (hovered != nullptr)
            {
                hovered->RenderAnchors();
            }
            terminal_refresh();

            /*while (selected == nullptr)
            {
                if (terminal_has_input())
                {
                    switch (terminal_read())
                    {
                        case GLFW_RAW_MOUSE_MOTION:
                        {
                            terminal_clear();
                            gameWindow->UpdateLayout({ 0, 0, (short)terminal_width(), (short)terminal_height });
                            gameWindow->RenderSelection();
                            Window* hovered = gameWindow->GetSelectedWindow();
                            if (hovered != nullptr)
                            {
                                hovered->RenderAnchors();
                            }
                            terminal_refresh();
                            break;
                        }
                        case GLFW_MOUSE_BUTTON_LEFT:
                            selected = gameWindow->GetSelectedWindow();
                            break;
                    }
                }
            }*/

            bool moving = true;
            while (moving)
            {
                terminal_clear();
                gameWindow->UpdateLayout({ 0, 0, (short)terminal_width(), (short)terminal_height() });
                gameWindow->RenderContent(0);
                terminal_refresh();

                if (terminal_has_input())
                {
                    if (terminal_get_key(EKey::LEFT_SHIFT))
                    {
                        switch (terminal_peek())
                        {
                            case EKey::UP:
                                selected->m_anchors.minYOffset--;
                                break;
                            case EKey::DOWN:
                                selected->m_anchors.maxYOffset++;
                                break;
                            case EKey::LEFT:
                                selected->m_anchors.minXOffset--;
                                break;
                            case EKey::RIGHT:
                                selected->m_anchors.maxXOffset++;
                                break;
                        }
                    }
                    else if (terminal_get_key(EKey::LEFT_CONTROL))
                    {
                        switch (terminal_peek())
                        {
                        case EKey::UP:
                            selected->m_anchors.maxYOffset--;
                            break;
                        case EKey::DOWN:
                            selected->m_anchors.minYOffset++;
                            break;
                        case EKey::LEFT:
                            selected->m_anchors.maxXOffset--;
                            break;
                        case EKey::RIGHT:
                            selected->m_anchors.minXOffset++;
                            break;
                        }
                    }
                    else
                    {
                        switch (terminal_peek())
                        {
                            case EKey::UP:
                                selected->m_anchors.minYOffset--;
                                selected->m_anchors.maxYOffset--;
                                break;
                            case EKey::DOWN:
                                selected->m_anchors.minYOffset++;
                                selected->m_anchors.maxYOffset++;
                                break;
                            case EKey::LEFT:
                                selected->m_anchors.minXOffset--;
                                selected->m_anchors.maxXOffset--;
                                break;
                            case EKey::RIGHT:
                                selected->m_anchors.minXOffset++;
                                selected->m_anchors.maxXOffset++;
                                break;
                        }
                    }

                    switch (terminal_read())
                    {
                    case EKey::X: {
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
                    case EKey::Y: {
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
                    case EKey::Q:
                        moving = false;
                        uiManager.CreateSettingsForAllWindows();
                        uiManager.SaveSettings();
                        break;
                    case EKey::P:
                        /*while (true)
                        {
                            terminal_clear();
                            gameWindow->UpdateLayout({ 0, 0, (short)terminal_width(), (short)terminal_width() });
                            gameWindow->RenderSelection();
                            Window* hovered = gameWindow->GetSelectedWindow();
                            if (hovered != nullptr)
                            {
                                hovered->RenderAnchors();
                            }
                            terminal_refresh();

                            if (terminal_has_input() && terminal_read() == GLFW_MOUSE_BUTTON_LEFT)
                            {
                                selected->SetParent(hovered);
                                break;
                            }
                        }*/
                        break;
                    }
                }

            }
        }

        k = EKey::G;

        //Handle output
        if (game.HasNextOutput())
        {
            ROGUE_PROFILE_SECTION("Handle Output");
            Output output = game.PopNextOutput();

            switch (output.m_type)
            {
            case GameReady:
                gameReady = true;
                break;
            case ViewUpdated:
                playerData.UpdateViewPlayer(output.Get<ViewUpdated>());
                break;
            default:
                // Unhandled case! Either invalid, or this output type needs to be handled.
                HALT();
                break;
            }
        }

        //Catch if the game is still loading - don't render until then!
        if (!gameReady)
        {
            continue;
        }

        terminal_clear();

        auto currentTime = chrono::system_clock::now();
        auto frameTime = chrono::duration_cast<chrono::duration<float>>(currentTime - clock);

        //Update UI Layout
        gameWindow->UpdateLayout({ 0, 0, (short)terminal_width(), (short)terminal_height() });

        switch (drawState)
        {
        case EDrawState::Normal:
            Render_Player_Data(playerData, renderWindow, playerLoc);
            break;
        case EDrawState::Temperature:
            Render_Temperature(playerData, renderWindow, playerLoc);
            break;
        }
        

        //LOS::Calculate(view, playerLoc, lookDirection, maxPass);
        //memory->Update(view);

        /*if (renderFlags & render)
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
        }*/

        renderWindow->Put(renderWindow->m_rect.w / 2, renderWindow->m_rect.h / 2, '@', Color(255, 0, 255), Color(0, 0, 0));
        xbar->SetColor(Color(255, 100, 0));
        //xbar->SetFill(((float)playerLoc.x()) / (map->m_size.x - 1), 0.03f);
        xlabel->SetString("x:");
        //xlabel->SetAlignment(EKey::ALIGN_DEFAULT);

        ybar->SetColor(Color(255, 100, 0));
        //ybar->SetFill(((float)playerLoc.y()) / (map->m_size.y - 1), 0.03f);
        ylabel->SetString("y:");
        //ylabel->SetAlignment(EKey::ALIGN_DEFAULT);
        gameWindow->RenderContent(frameTime.count());

        char fpsString[20];
        std::snprintf(fpsString, 20, "%f", 1.0f / frameTime.count());
        terminal_print(0, 0, fpsString);

        char locString[20];
        std::snprintf(locString, 20, "%d, %d", playerData.m_memory.m_localPosition.x, playerData.m_memory.m_localPosition.y);
        terminal_print(1, 1, locString);

        {
            ROGUE_PROFILE_SECTION("Render");
            terminal_refresh();
        }
        clock = currentTime;

        ROGUE_PROFILE_FRAME();
    }

    //Cleanup phase
    game.CreateInput<SaveAndExit>();
    gameThread.join();
    terminal_close();
    Jobs::Shutdown();
    ResourceManager::ShutdownResources();

    return EXIT_SUCCESS;
}
