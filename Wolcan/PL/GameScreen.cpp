#include "GameScreen.h"
#include "GameArt.h"
#include "WindowControls.h"
#include "raylib.h"

#include <algorithm>
#include <cmath>

void resetGameVisuals(GameVisualState& state, const GameLogic& game)
{
    state = GameVisualState{};
    state.playerRow = static_cast<float>(game.getPlayer().getRow());
    state.playerColumn = static_cast<float>(game.getPlayer().getColumn());
}

void runGame(GameLogic& game)
{
    GameArt gameArt;
    GameVisualState visualState;
    resetGameVisuals(visualState, game);
    float startRow = visualState.playerRow;
    float startColumn = visualState.playerColumn;
    float progress = 1.0f;
    float repeatCooldown = 0.0f;
    int bufferedDirection = -1;
    int previousHeldDirection = -1;
    int keys[4] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
    int rowOffsets[4] = {-1, 1, 0, 0};
    int columnOffsets[4] = {0, 0, -1, 1};
    const int directionCount = 4;

    while (!WindowShouldClose())
    {
        handleWindowShortcuts();
        float rawFrameTime = GetFrameTime();
        float animationTime = std::clamp(rawFrameTime, 0.0f, 0.05f);
        bool returnToMenu = IsKeyPressed(KEY_ESCAPE);
        if (!IsWindowFocused()) { game.setPaused(true); }
        if (!returnToMenu) { game.updateTime(static_cast<double>(rawFrameTime)); }
        if (IsKeyPressed(KEY_P) && IsWindowFocused()) { game.setPaused(!game.isPaused()); }

        bool changedLevel = false;
        if (IsKeyPressed(KEY_R)) { game.restart(false); changedLevel = true; }
        else if (IsKeyPressed(KEY_T)) { game.restart(true); changedLevel = true; }
        else if (IsKeyPressed(KEY_N)) { changedLevel = game.nextLevel(); }
        if (changedLevel)
        {
            resetGameVisuals(visualState, game);
            startRow = visualState.playerRow;
            startColumn = visualState.playerColumn;
            progress = 1.0f;
            repeatCooldown = 0.0f;
            bufferedDirection = -1;
            previousHeldDirection = -1;
        }
        else if (!game.isPaused())
        {
            visualState.idleTime += animationTime;
            repeatCooldown = std::max(0.0f, repeatCooldown - animationTime);
            progress = std::min(1.0f, progress + animationTime / 0.105f);
            visualState.playerRow = startRow + (static_cast<float>(game.getPlayer().getRow()) - startRow) * progress;
            visualState.playerColumn = startColumn + (static_cast<float>(game.getPlayer().getColumn()) - startColumn) * progress;
            visualState.moving = progress < 1.0f;
            if (visualState.moving) { visualState.walkPhase = std::fmod(visualState.walkPhase + animationTime * 18.0f, 6.2831853f); }

            int heldDirection = -1;
            for (int directionIndex = 0; directionIndex < directionCount; directionIndex++)
            {
                if (IsKeyPressed(keys[directionIndex])) { bufferedDirection = directionIndex; }
                if (IsKeyDown(keys[directionIndex])) { heldDirection = directionIndex; }
            }
            // Finish each cardinal tween before accepting a turn. Buffered input
            // stays responsive without visually cutting diagonally through a wall.
            if (!returnToMenu && !game.hasWon() && progress >= 1.0f)
            {
                int direction = bufferedDirection >= 0 ? bufferedDirection : heldDirection;
                bool requested = bufferedDirection >= 0 || heldDirection != previousHeldDirection || repeatCooldown <= 0.0f;
                if (direction >= 0 && requested)
                {
                    visualState.facing = direction;
                    if (game.tryMovePlayer(rowOffsets[direction], columnOffsets[direction]))
                    {
                        startRow = visualState.playerRow;
                        startColumn = visualState.playerColumn;
                        progress = 0.0f;
                        visualState.moving = true;
                    }
                    repeatCooldown = 0.12f;
                    bufferedDirection = -1;
                }
            }
            previousHeldDirection = heldDirection;
        }

        BeginDrawing();
        ClearBackground({7, 8, 12, 255});
        gameArt.draw(game, visualState);
        EndDrawing();
        if (returnToMenu) { return; }
    }
}
