#include "MainMenu.h"

#include "raylib.h"

void drawMainMenu(int selectedSize)
{
    int panelLeft = (GetScreenWidth() - 620) / 2;
    int panelTop = (GetScreenHeight() - 500) / 2 - 10;
    DrawRectangle(panelLeft, panelTop, 620, 500, DARKBLUE);
    DrawText("WOLCAN", panelLeft + 60, panelTop + 40, 64, RAYWHITE);
    DrawText("Find your way through the maze", panelLeft + 60, panelTop + 120, 24, SKYBLUE);
    DrawText("MAZE SIZE", panelLeft + 60, panelTop + 200, 22, RAYWHITE);
    DrawText(TextFormat("<   %i x %i   >", selectedSize, selectedSize),
        panelLeft + 60, panelTop + 245, 40, GOLD);
    DrawText("Left / Right: choose a size from 10 to 50", panelLeft + 60, panelTop + 305, 21, LIGHTGRAY);
    DrawText("ENTER  Start game", panelLeft + 60, panelTop + 375, 26, RAYWHITE);
    DrawText("ESC     Quit", panelLeft + 60, panelTop + 420, 26, LIGHTGRAY);
    DrawText("Starter edition | Four directions | One reachable exit", 200, GetScreenHeight() - 35, 20, GRAY);
}

int runMainMenu()
{
    int selectedSize = 10;
    while (!WindowShouldClose())
    {
        // Bound the menu selection; BLL independently validates its constructor input.
        if (IsKeyPressed(KEY_LEFT) && selectedSize > 10)
        {
            selectedSize--;
        }
        else if (IsKeyPressed(KEY_RIGHT) && selectedSize < 50)
        {
            selectedSize++;
        }

        if (IsKeyPressed(KEY_ENTER))
        {
            return selectedSize;
        }
        if (IsKeyPressed(KEY_ESCAPE))
        {
            return 0;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        drawMainMenu(selectedSize);
        EndDrawing();
    }
    return 0;
}
