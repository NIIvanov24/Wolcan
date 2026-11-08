#pragma once

#include "GameLogic.h"
#include "raylib.h"

struct GameVisualState
{
    float playerRow = 0.0f;
    float playerColumn = 0.0f;
    float walkPhase = 0.0f;
    float idleTime = 0.0f;
    int facing = 1;
    bool moving = false;
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
    GameArt();
    ~GameArt();
    GameArt(const GameArt& other) = delete;
    GameArt& operator=(const GameArt& other) = delete;

    void draw(const GameLogic& game, const GameVisualState& visualState) const;

private:
    Font titleFont = {};
    Font bodyFont = {};
    bool ownsTitleFont = false;
    bool ownsBodyFont = false;

    void label(const GameViewLayout& layout, const char* text, float x, float y,
        float size, Color color, bool pixelFont = false) const;
    void centered(const GameViewLayout& layout, const char* text, float centerX,
        float y, float size, Color color, bool pixelFont = false) const;
    void drawHud(const GameLogic& game, const GameViewLayout& layout) const;
    void drawMap(const GameLogic& game, const GameVisualState& state,
        const GameViewLayout& layout) const;
    void drawOverlay(const GameLogic& game, const GameViewLayout& layout) const;
};
