#pragma once

#include "GameLogic.h"
#include "Progression.h"
#include "raylib.h"

struct GameVisualState
{
    float playerRow = 0.0f;
    float playerColumn = 0.0f;
    float walkPhase = 0.0f;
    float idleTime = 0.0f;
    int facing = 1;
    bool moving = false;
    float displayedHealth = 100.0f;
    float damageFlash = 0.0f;
    float pickupFlash = 0.0f;
    float resultTime = 0.0f;
    float toastTime = 0.0f;
    int toastAchievement = -1;
    RunRewards rewards = {};
};

struct GameViewLayout
{
    Rectangle viewport;
    Rectangle hud;
    Rectangle world;
    Rectangle map;
    float scale;
    int tileSize;
    float cameraX;
    float cameraY;
};

GameViewLayout createGameViewLayout(int screenWidth, int screenHeight,
    const GameLogic& game, const GameVisualState& visualState);

class GameArt
{
public:
    explicit GameArt(Skin skin = Skin::Explorer);
    ~GameArt();
    GameArt(const GameArt& other) = delete;
    GameArt& operator=(const GameArt& other) = delete;

    void draw(const GameLogic& game, const GameVisualState& visualState,
        const Progression* progression = nullptr) const;

private:
    Texture2D playerTexture = {};
    Font titleFont = {};
    Font bodyFont = {};
    bool ownsTitleFont = false;
    bool ownsBodyFont = false;

    void label(const GameViewLayout& layout, const char* text, float x, float y,
        float size, Color color, bool pixelFont = false) const;
    void centered(const GameViewLayout& layout, const char* text, float centerX,
        float y, float size, Color color, bool pixelFont = false) const;
    void drawHud(const GameLogic& game, const GameVisualState& state, const GameViewLayout& layout) const;
    void drawMap(const GameLogic& game, const GameVisualState& state,
        const GameViewLayout& layout) const;
    void drawOverlay(const GameLogic& game, const GameVisualState& state,
        const GameViewLayout& layout, const Progression* progression) const;
    void drawAchievementToast(const GameVisualState& state, const GameViewLayout& layout,
        const Progression& progression) const;
};
