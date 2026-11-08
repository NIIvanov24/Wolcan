#include "MainMenu.h"
#include "GameScreen.h"
#include "raylib.h"

#include <exception>
#include <iostream>

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 800;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Wolcan - Volcanic Isle");
    if (!IsWindowReady())
    {
        std::cerr << "The game window could not be initialized." << std::endl;
        return 1;
    }

    int monitor = GetCurrentMonitor();
    int availableWidth = GetMonitorWidth(monitor) - 80;
    int availableHeight = GetMonitorHeight(monitor) - 80;
    int windowWidth = screenWidth < availableWidth ? screenWidth : availableWidth;
    int windowHeight = screenHeight < availableHeight ? screenHeight : availableHeight;
    if (windowWidth < 900) { windowWidth = 900; }
    if (windowHeight < 640) { windowHeight = 640; }
    SetWindowSize(windowWidth, windowHeight);
    SetWindowMinSize(900, 640);

    // Escape is handled by each screen: back to menu in-game, quit in menu.
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    int exitCode = 0;
    try
    {
        while (!WindowShouldClose())
        {
            int selectedSize = runMainMenu();
            if (selectedSize == 0 || WindowShouldClose())
            {
                break;
            }

            GameLogic game(selectedSize, selectedSize);
            runGame(game);
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "The game stopped: " << error.what() << std::endl;
        exitCode = 1;
    }

    CloseWindow();
    return exitCode;
}
