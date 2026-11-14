#include "GameScreen.h"
#include "GameArt.h"
#include "GameAudio.h"
#include "GameSettings.h"
#include "WindowControls.h"
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <vector>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace
{
bool gameHasFocus()
{
#if defined(__EMSCRIPTEN__)
    return EM_ASM_INT({ return document.hasFocus() && !document.hidden; }) != 0;
#else
    return IsWindowFocused();
#endif
}

struct AudibleTrap
{
    GridPosition position;
    HazardKind kind;
    HazardPhase lastPhase;
};

std::vector<AudibleTrap> collectAudibleTraps(const GameLogic& game)
{
    std::vector<AudibleTrap> traps;
    for (int row = 0; row < game.getMaze().getRowCount(); row++)
    {
        for (int column = 0; column < game.getMaze().getColumnCount(); column++)
        {
            CellFeature tile = game.getCellFeature(row, column);
            if (tile.hazard != HazardKind::None) { traps.push_back({{row, column}, tile.hazard, tile.phase}); }
        }
    }
    return traps;
}

void playTrapCues(GameAudio& audio, const GameLogic& game, std::vector<AudibleTrap>& traps)
{
    bool cracking = false;
    bool collapse = false;
    bool spikes = false;
    int count = static_cast<int>(traps.size());
    for (int index = 0; index < count; index++)
    {
        auto& trap = traps[index];
        int distance = std::abs(trap.position.row - game.getPlayer().getRow())
            + std::abs(trap.position.column - game.getPlayer().getColumn());
        CellFeature tile = game.getCellFeature(trap.position.row, trap.position.column);
        if (distance <= 6 && trap.lastPhase != tile.phase)
        {
            if (trap.kind == HazardKind::Fragile && tile.phase == HazardPhase::Warning) { cracking = true; }
            if (tile.hazard == HazardKind::Lava && tile.phase == HazardPhase::Active) { collapse = true; }
            if (trap.kind == HazardKind::Spikes && tile.phase == HazardPhase::Active) { spikes = true; }
        }
        trap.lastPhase = tile.phase;
    }
    if (cracking) { audio.play(SoundCue::Cracking); }
    if (collapse) { audio.play(SoundCue::Collapse); }
    if (spikes) { audio.play(SoundCue::Spikes); }
}
}

void resetGameVisuals(GameVisualState& state, const GameLogic& game)
{
    state = GameVisualState{};
    state.playerRow = static_cast<float>(game.getPlayer().getRow());
    state.playerColumn = static_cast<float>(game.getPlayer().getColumn());
    state.displayedHealth = static_cast<float>(game.getHealth());
}

void runGame(GameLogic& game, Progression* progression, const GameSettings* settings, GameAudio* audio)
{
    Progression sessionProfile;
    Progression& profile = progression != nullptr ? *progression : sessionProfile;
    const GameSettings defaults;
    const GameSettings& bindings = settings != nullptr ? *settings : defaults;
    KonamiInput konami;
    std::vector<AudibleTrap> audibleTraps;
    if (audio != nullptr) { audibleTraps = collectAudibleTraps(game); }
    auto play = [audio](SoundCue cue) { if (audio != nullptr) { audio->play(cue); } };
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
    bool hammerBuffered = false;
    int previousHeldDirection = -1;
    int keys[4] = {bindings.key(GameAction::Up), bindings.key(GameAction::Down),
        bindings.key(GameAction::Left), bindings.key(GameAction::Right)};
    int rowOffsets[4] = {-1, 1, 0, 0};
    int columnOffsets[4] = {0, 0, -1, 1};
    const int directionCount = 4;
    float damageSoundCooldown = 0.0f;
    bool medalSoundPending = false;

    while (!WindowShouldClose())
    {
        handleWindowShortcuts(&bindings);
        if (audio != nullptr)
        {
            audio->update(bindings.musicVolume, bindings.effectsVolume, bindings.musicMuted, bindings.effectsMuted);
        }
        float rawFrameTime = GetFrameTime();
        float animationTime = std::clamp(rawFrameTime, 0.0f, 0.05f);
        bool returnToMenu = IsKeyPressed(bindings.key(GameAction::Menu));
        bool focused = gameHasFocus();
        if (!focused) { game.setPaused(true); }
        double damageBefore = game.getDamageTaken();
        bool wasInLava = game.isInLava();
        bool wasDying = game.isDying();
        if (!returnToMenu) { game.updateTime(static_cast<double>(rawFrameTime)); }
        bool respawned = attemptRevision != game.getAttemptRevision();
        if (IsKeyPressed(bindings.key(GameAction::Pause)) && focused) { game.setPaused(!game.isPaused()); }

        bool changedLevel = false;
        if (IsKeyPressed(bindings.key(GameAction::Restart))) { game.restart(false); changedLevel = true; }
        else if (IsKeyPressed(bindings.key(GameAction::Replay))) { game.restart(true); changedLevel = true; }
        else if (IsKeyPressed(bindings.key(GameAction::NextLevel))) { changedLevel = game.nextLevel(); }
        int pressedKey = GetKeyPressed();
        while (pressedKey != KEY_NULL)
        {
            if (!returnToMenu && !changedLevel && !game.isPaused() && !game.hasWon() && !game.isDying()
                && konami.press(pressedKey, bindings) && game.grantHammer())
            {
                std::uint32_t unlocked = profile.recordKonamiCode();
                pendingToasts |= unlocked;
                attemptAchievements |= unlocked;
                visualState.hammerGrantFlash = 1.0f;
                if (unlocked == 0) { play(SoundCue::Achievement); }
            }
            pressedKey = GetKeyPressed();
        }
        if (changedLevel || attemptRevision != game.getAttemptRevision())
        {
            attemptRevision = game.getAttemptRevision();
            attemptAchievements = 0;
            rewardsRecorded = false;
            medalSoundPending = false;
            resetGameVisuals(visualState, game);
            konami = {};
            if (respawned && !changedLevel)
            {
                visualState.respawnFlash = 1.0f;
                play(SoundCue::Respawn);
            }
            if (audio != nullptr) { audibleTraps = collectAudibleTraps(game); }
            startRow = visualState.playerRow;
            startColumn = visualState.playerColumn;
            progress = 1.0f;
            stepDuration = 0.105f;
            repeatCooldown = 0.0f;
            bufferedDirection = -1;
            hammerBuffered = false;
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
            if (IsKeyPressed(bindings.key(GameAction::Hammer)) && game.getHammerUses() > 0)
            {
                hammerBuffered = true;
            }

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
                bool swung = hammerBuffered;
                hammerBuffered = false;
                if (swung && game.getHammerUses() > 0)
                {
                    int facing = bufferedDirection >= 0 ? bufferedDirection : visualState.facing;
                    visualState.facing = facing;
                    bufferedDirection = -1;
                    if (game.tryBreakWall(rowOffsets[facing], columnOffsets[facing]))
                    {
                        visualState.hammerStrikePosition = {game.getPlayer().getRow() + rowOffsets[facing],
                            game.getPlayer().getColumn() + columnOffsets[facing]};
                        visualState.hammerStrikeFlash = 1.0f;
                        std::uint32_t unlocked = profile.recordHammerBreak();
                        attemptAchievements |= unlocked;
                        pendingToasts |= unlocked;
                        play(SoundCue::Hammer);
                    }
                }
                int direction = bufferedDirection >= 0 ? bufferedDirection : heldDirection;
                bool requested = bufferedDirection >= 0 || heldDirection != previousHeldDirection || repeatCooldown <= 0.0f;
                if (!swung && direction >= 0 && requested)
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
                        play(SoundCue::Step);
                        std::uint32_t unlocked = 0;
                        if (game.getItemsCollected() > itemsBefore)
                        {
                            GridPosition position = game.getLastCollectedPosition();
                            unlocked |= profile.recordItem(game.getRecordKey(), position.row, position.column);
                            visualState.pickupFlash = 1.0f;
                            play(SoundCue::Collect);
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
        visualState.hammerGrantFlash = std::max(0.0f, visualState.hammerGrantFlash - animationTime / 1.6f);
        visualState.hammerStrikeFlash = std::max(0.0f, visualState.hammerStrikeFlash - animationTime / 0.45f);
        visualState.lavaEnterFlash = std::max(0.0f, visualState.lavaEnterFlash - animationTime / 0.5f);
        visualState.lavaExitFlash = std::max(0.0f, visualState.lavaExitFlash - animationTime / 0.45f);
        visualState.respawnFlash = std::max(0.0f, visualState.respawnFlash - animationTime / 0.65f);
        if (!changedLevel && !respawned && game.isInLava() != wasInLava)
        {
            if (game.isInLava()) { visualState.lavaEnterFlash = 1.0f; play(SoundCue::Lava); }
            else { visualState.lavaExitFlash = 1.0f; }
        }
        if (!changedLevel && !respawned && !wasDying && game.isDying()) { play(SoundCue::Death); }
        if (audio != nullptr && !game.isPaused()) { playTrapCues(*audio, game, audibleTraps); }
        damageSoundCooldown = std::max(0.0f, damageSoundCooldown - animationTime);
        if (!changedLevel && !respawned && game.getDamageTaken() > damageBefore && damageSoundCooldown <= 0.0f)
        {
            visualState.damageFlash = 1.0f;
            play(SoundCue::Damage);
            damageSoundCooldown = 0.48f;
        }
        if (game.hasWon())
        {
            visualState.resultTime += animationTime;
            if (!rewardsRecorded)
            {
                RunSummary summary{game.getRecordKey(), game.getMaze().getDifficulty(), game.getElapsedSeconds(),
                    game.getMoveCount(), game.getShortestPath(), game.getHealth(), game.getDamageTaken(), game.isAssisted()};
                visualState.rewards = profile.recordFinish(summary);
                pendingToasts |= visualState.rewards.newAchievements;
                visualState.rewards.newAchievements |= attemptAchievements;
                rewardsRecorded = true;
                play(SoundCue::Victory);
                medalSoundPending = !game.isAssisted();
            }
            if (medalSoundPending && visualState.resultTime >= 0.62f)
            {
                play(SoundCue::Medal);
                medalSoundPending = false;
            }
        }
        if (activeToast >= 0)
        {
            toastTime += animationTime;
            if (toastTime >= 4.0f) { activeToast = -1; }
        }
        if (activeToast < 0 && pendingToasts != 0 && (!game.hasWon() || visualState.resultTime >= 1.1f))
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
                    play(SoundCue::Achievement);
                    break;
                }
            }
        }
        visualState.toastAchievement = activeToast;
        visualState.toastTime = toastTime;

        BeginDrawing();
        ClearBackground({7, 8, 12, 255});
        gameArt.draw(game, visualState, &profile, &bindings);
        EndDrawing();
        if (returnToMenu) { return; }
    }
}
