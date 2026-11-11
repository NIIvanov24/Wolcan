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
    state.displayedHealth = static_cast<float>(game.getHealth());
}

void runGame(GameLogic& game, Progression* progression)
{
    Progression sessionProfile;
    Progression& profile = progression != nullptr ? *progression : sessionProfile;
    GameArt gameArt(profile.getSelectedSkin());
    GameVisualState visualState;
    resetGameVisuals(visualState, game);
    float startRow = visualState.playerRow;
    float startColumn = visualState.playerColumn;
    float progress = 1.0f;
    float stepDuration = 0.105f;
    float repeatCooldown = 0.0f;
    unsigned int attemptRevision = game.getAttemptRevision();
    std::uint32_t attemptAchievements = 0;
    std::uint32_t pendingToasts = 0;
    int activeToast = -1;
    float toastTime = 0.0f;
    bool rewardsRecorded = false;
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
        double damageBefore = game.getDamageTaken();
        if (!returnToMenu) { game.updateTime(static_cast<double>(rawFrameTime)); }
        if (IsKeyPressed(KEY_P) && IsWindowFocused()) { game.setPaused(!game.isPaused()); }

        bool changedLevel = false;
        if (IsKeyPressed(KEY_R)) { game.restart(false); changedLevel = true; }
        else if (IsKeyPressed(KEY_T)) { game.restart(true); changedLevel = true; }
        else if (IsKeyPressed(KEY_N)) { changedLevel = game.nextLevel(); }
        if (changedLevel || attemptRevision != game.getAttemptRevision())
        {
            attemptRevision = game.getAttemptRevision();
            attemptAchievements = 0;
            rewardsRecorded = false;
            resetGameVisuals(visualState, game);
            startRow = visualState.playerRow;
            startColumn = visualState.playerColumn;
            progress = 1.0f;
            stepDuration = 0.105f;
            repeatCooldown = 0.0f;
            bufferedDirection = -1;
            previousHeldDirection = -1;
        }
        else if (!game.isPaused())
        {
            visualState.idleTime += animationTime;
            repeatCooldown = std::max(0.0f, repeatCooldown - animationTime);
            progress = std::min(1.0f, progress + animationTime / stepDuration);
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
            if (!returnToMenu && !game.hasWon() && !game.isDying() && progress >= 1.0f)
            {
                int direction = bufferedDirection >= 0 ? bufferedDirection : heldDirection;
                bool requested = bufferedDirection >= 0 || heldDirection != previousHeldDirection || repeatCooldown <= 0.0f;
                if (direction >= 0 && requested)
                {
                    visualState.facing = direction;
                    int itemsBefore = game.getItemsCollected();
                    int escapesBefore = game.getLavaEscapes();
                    float moveSpeed = game.getMovementMultiplier();
                    if (game.tryMovePlayer(rowOffsets[direction], columnOffsets[direction]))
                    {
                        moveSpeed = std::min(moveSpeed, game.getMovementMultiplier());
                        startRow = visualState.playerRow;
                        startColumn = visualState.playerColumn;
                        stepDuration = 0.105f / moveSpeed;
                        progress = 0.0f;
                        visualState.moving = true;
                        std::uint32_t unlocked = 0;
                        if (game.getItemsCollected() > itemsBefore)
                        {
                            GridPosition position = game.getLastCollectedPosition();
                            unlocked |= profile.recordItem(game.getRecordKey(), position.row, position.column);
                            visualState.pickupFlash = 1.0f;
                        }
                        if (game.getLavaEscapes() > escapesBefore) { unlocked |= profile.recordLavaEscape(); }
                        attemptAchievements |= unlocked;
                        pendingToasts |= unlocked;
                    }
                    repeatCooldown = 0.12f / moveSpeed;
                    bufferedDirection = -1;
                }
            }
            previousHeldDirection = heldDirection;
        }

        visualState.displayedHealth += (static_cast<float>(game.getHealth()) - visualState.displayedHealth)
            * std::min(1.0f, animationTime * 9.0f);
        visualState.damageFlash = std::max(0.0f, visualState.damageFlash - animationTime * 3.0f);
        visualState.pickupFlash = std::max(0.0f, visualState.pickupFlash - animationTime * 2.4f);
        if (game.getDamageTaken() > damageBefore) { visualState.damageFlash = 1.0f; }
        if (game.hasWon())
        {
            visualState.resultTime += animationTime;
            if (!rewardsRecorded)
            {
                RunSummary summary{game.getRecordKey(), game.getMaze().getDifficulty(), game.getElapsedSeconds(),
                    game.getMoveCount(), game.getShortestPath(), game.getHealth(), game.getDamageTaken()};
                visualState.rewards = profile.recordFinish(summary);
                pendingToasts |= visualState.rewards.newAchievements;
                visualState.rewards.newAchievements |= attemptAchievements;
                rewardsRecorded = true;
            }
        }
        if (activeToast >= 0)
        {
            toastTime += animationTime;
            if (toastTime >= 4.0f) { activeToast = -1; }
        }
        if (activeToast < 0 && pendingToasts != 0)
        {
            int achievementCount = profile.getAchievementCount();
            for (int index = 0; index < achievementCount; index++)
            {
                std::uint32_t bit = 1u << index;
                if ((pendingToasts & bit) != 0)
                {
                    activeToast = index;
                    pendingToasts &= ~bit;
                    toastTime = 0.0f;
                    break;
                }
            }
        }
        visualState.toastAchievement = activeToast;
        visualState.toastTime = toastTime;

        BeginDrawing();
        ClearBackground({7, 8, 12, 255});
        gameArt.draw(game, visualState, &profile);
        EndDrawing();
        if (returnToMenu) { return; }
    }
}
