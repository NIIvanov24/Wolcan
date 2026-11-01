#include "MainMenu.h"
#include "GameScreen.h"
#include "raylib.h"

#include <exception>
#include <iostream>

int main()
{
    const int screenWidth = 1000;
    const int screenHeight = 640;
    InitWindow(screenWidth, screenHeight, "Wolcan - Maze Starter");
    if (!IsWindowReady())
    {
        std::cerr << "The game window could not be initialized." << std::endl;
        return 1;
    }

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
