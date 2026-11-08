#include "GameScreen.h"
#include "WindowControls.h"
#include "raylib.h"

void handleMovement(GameLogic& game)
{
    // A single key press requests one cell. BLL decides whether it is legal.
    if (IsKeyPressed(KEY_UP))
    {
        game.tryMovePlayer(-1, 0);
    }
    else if (IsKeyPressed(KEY_DOWN))
    {
        game.tryMovePlayer(1, 0);
    }
    else if (IsKeyPressed(KEY_LEFT))
    {
        game.tryMovePlayer(0, -1);
    }
    else if (IsKeyPressed(KEY_RIGHT))
    {
        game.tryMovePlayer(0, 1);
    }
}

void drawMaze(const GameLogic& game)
{
    const Maze& maze = game.getMaze();
    const Player& player = game.getPlayer();
    // Keep cells at least 10 pixels high at 50x50: raylib's default font
    // clamps smaller requests to 10 pixels.
    int gridAreaSize = GetScreenHeight() - 140;
    if (gridAreaSize > GetScreenWidth() - 80) { gridAreaSize = GetScreenWidth() - 80; }
    int largestDimension = maze.getRowCount() > maze.getColumnCount()
        ? maze.getRowCount() : maze.getColumnCount();
    int cellSize = gridAreaSize / largestDimension;
    int gridLeft = (GetScreenWidth() - maze.getColumnCount() * cellSize) / 2;
    int gridTop = 70;
    int fontSize = cellSize / 2;
    if (fontSize < 10)
    {
        fontSize = 10;
    }

    int rowCount = maze.getRowCount();
    int columnCount = maze.getColumnCount();
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        {
            int cellLeft = gridLeft + columnIndex * cellSize;
            int cellTop = gridTop + rowIndex * cellSize;
            char cellSymbol[2] = {maze.getCell(rowIndex, columnIndex), 0};
            Color cellColor = cellSymbol[0] == '#' ? DARKBLUE : DARKGRAY;
            Color textColor = cellSymbol[0] == '#' ? SKYBLUE : GRAY;

            if (rowIndex == rowCount - 1 && columnIndex == columnCount - 1)
            {
                cellSymbol[0] = 'E';
                cellColor = DARKGREEN;
                textColor = RAYWHITE;
            }
            if (rowIndex == player.getRow() && columnIndex == player.getColumn())
            {
                cellSymbol[0] = '@';
                cellColor = GOLD;
                textColor = BLACK;
            }

            DrawRectangle(cellLeft, cellTop, cellSize - 1, cellSize - 1, cellColor);
            int textLeft = cellLeft + (cellSize - MeasureText(cellSymbol, fontSize)) / 2;
            int textTop = cellTop + (cellSize - fontSize) / 2;
            DrawText(cellSymbol, textLeft, textTop, fontSize, textColor);
        }
    }
}

void drawGameScreen(const GameLogic& game)
{
    DrawText("WOLCAN", 30, 22, 30, SKYBLUE);
    DrawText(TextFormat("Position: %i, %i    Moves: %i",
        game.getPlayer().getRow(), game.getPlayer().getColumn(), game.getMoveCount()),
        410, 28, 22, RAYWHITE);
    drawMaze(game);
    DrawText("Arrows: move    R: restart    ESC: menu", 150, GetScreenHeight() - 68, 22, RAYWHITE);
    DrawText("@ Player    # Wall    . Passage    E Exit", 150, GetScreenHeight() - 35, 20, LIGHTGRAY);

    if (game.hasWon())
    {
        int panelTop = GetScreenHeight() / 2 - 90;
        DrawRectangle(210, panelTop, 580, 180, DARKBLUE);
        DrawRectangleLines(210, panelTop, 580, 180, GOLD);
        DrawText("MAZE COMPLETE!", 255, panelTop + 30, 38, GOLD);
        DrawText("R: play again    ESC: main menu", 255, panelTop + 105, 24, RAYWHITE);
    }
}

void runGame(GameLogic& game)
{
    while (!WindowShouldClose())
    {
        handleWindowShortcuts();
        bool shouldReturnToMenu = IsKeyPressed(KEY_ESCAPE);
        if (!shouldReturnToMenu && IsKeyPressed(KEY_R))
        {
            game.restart();
        }
        else if (!shouldReturnToMenu)
        {
            handleMovement(game);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        drawGameScreen(game);
        EndDrawing();

        // EndDrawing polls events. Finish this frame so the menu cannot reuse
        // the Escape press that requested this screen change.
        if (shouldReturnToMenu)
        {
            return;
        }
    }
}
