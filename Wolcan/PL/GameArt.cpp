#include "GameArt.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace
{
    Rectangle scaledRectangle(const GameViewLayout& layout, float x, float y, float width, float height)
    {
        return {std::round(layout.viewport.x + x * layout.scale),
            std::round(layout.viewport.y + y * layout.scale),
            std::round(width * layout.scale), std::round(height * layout.scale)};
    }

    void panel(const GameViewLayout& layout, float x, float y, float width, float height)
    {
        DrawRectangleRec(scaledRectangle(layout, x + 4, y + 5, width, height), {5, 4, 7, 180});
        DrawRectangleRec(scaledRectangle(layout, x, y, width, height), {82, 62, 53, 255});
        DrawRectangleRec(scaledRectangle(layout, x + 1, y + 1, width - 2, height - 2), {23, 22, 26, 255});
        DrawRectangleRec(scaledRectangle(layout, x + 1, y + 1, width - 2, 2), {105, 76, 53, 255});
    }

    void formatTime(double seconds, char* destination, std::size_t capacity)
    {
        if (seconds < 0.0)
        {
            std::snprintf(destination, capacity, "--:--");
            return;
        }
        double bounded = std::clamp(seconds, 0.0, 359999.99);
        int minutes = static_cast<int>(bounded / 60.0);
        int wholeSeconds = static_cast<int>(bounded) % 60;
        int hundredths = static_cast<int>((bounded - std::floor(bounded)) * 100.0);
        std::snprintf(destination, capacity, "%02i:%02i.%02i", minutes, wholeSeconds, hundredths);
    }

    unsigned int tileHash(int row, int column)
    {
        unsigned int value = static_cast<unsigned int>(row) * 374761393U
            + static_cast<unsigned int>(column) * 668265263U + 12031U;
        value = (value ^ (value >> 13)) * 1274126177U;
        return value ^ (value >> 16);
    }

    void tilePixel(float x, float y, float pixel, float left, float top, float width, float height, Color color)
    {
        DrawRectangleRec({std::round(x + left * pixel), std::round(y + top * pixel),
            std::max(1.0f, std::round(width * pixel)), std::max(1.0f, std::round(height * pixel))}, color);
    }

    void drawFloor(float x, float y, int tileSize, unsigned int hash, char cell)
    {
        float pixel = static_cast<float>(tileSize) / 16.0f;
        unsigned char variation = static_cast<unsigned char>(hash % 8U);
        DrawRectangleRec({x, y, static_cast<float>(tileSize), static_cast<float>(tileSize)},
            {static_cast<unsigned char>(48 + variation), static_cast<unsigned char>(34 + variation),
                static_cast<unsigned char>(28 + variation), 255});
        if (hash % 3U == 0U)
        {
            tilePixel(x, y, pixel, 2, 12, 3, 1, {31, 26, 27, 150});
            tilePixel(x, y, pixel, 5, 13, 1, 2, {31, 26, 27, 150});
        }
        float fleckX = 2.0f + static_cast<float>(hash % 10U);
        float fleckY = 3.0f + static_cast<float>((hash >> 5) % 9U);
        tilePixel(x, y, pixel, fleckX, fleckY, 2, 1, {99, 68, 45, 170});
        tilePixel(x, y, pixel, 3, 12, 1, 1, {27, 25, 29, 190});
        if (cell == ',' || cell == '+')
        {
            tilePixel(x, y, pixel, 7, 7, 2, 2, {112, 120, 99, 145});
            tilePixel(x, y, pixel, 7, 7, 1, 1, {167, 154, 111, 130});
        }
        if (cell == '+')
        {
            tilePixel(x, y, pixel, 1, 4, 2, 1, {60, 154, 145, 210});
            tilePixel(x, y, pixel, 13, 11, 2, 1, {60, 154, 145, 210});
        }
    }

    void drawWall(const Maze& maze, int row, int column, float x, float y, int tileSize, float time)
    {
        unsigned int hash = tileHash(row, column);
        float pixel = static_cast<float>(tileSize) / 16.0f;
        unsigned char shade = static_cast<unsigned char>(hash % 9U);
        drawFloor(x, y, tileSize, hash, '.');
        bool openLeft = maze.isWalkable(row, column - 1);
        bool openRight = maze.isWalkable(row, column + 1);
        bool openTop = maze.isWalkable(row - 1, column);
        float left = openLeft ? 1.0f : 0.0f;
        float right = openRight ? 15.0f : 16.0f;
        float top = openTop ? 1.0f : 0.0f;
        Color face = {static_cast<unsigned char>(43 + shade), static_cast<unsigned char>(39 + shade),
            static_cast<unsigned char>(43 + shade), 255};
        tilePixel(x, y, pixel, left, top + 2, right - left, 14 - top, face);
        tilePixel(x, y, pixel, left + 1, top, right - left - 2, 12 - top,
            {static_cast<unsigned char>(72 + shade), static_cast<unsigned char>(59 + shade),
                static_cast<unsigned char>(56 + shade), 255});
        tilePixel(x, y, pixel, left, top + 3, right - left, 7,
            {static_cast<unsigned char>(66 + shade), static_cast<unsigned char>(54 + shade),
                static_cast<unsigned char>(53 + shade), 255});
        float ridge = 2.0f + static_cast<float>((hash >> 4) % 3U);
        tilePixel(x, y, pixel, left + 2, top + 1, 7, 1, {121, 95, 76, 230});
        tilePixel(x, y, pixel, 9, ridge, 4, 1, {103, 79, 68, 220});
        tilePixel(x, y, pixel, left + 1, top + 2, 1, 5, {99, 81, 73, 220});
        tilePixel(x, y, pixel, right - 2, 6, 1, 6, {29, 26, 32, 230});
        tilePixel(x, y, pixel, 3, 11, 9, 2, {31, 27, 33, 180});
        if (hash % 3U == 1U)
        {
            tilePixel(x, y, pixel, 4, 5, 4, 2, {79, 67, 63, 255});
            tilePixel(x, y, pixel, 8, 6, 1, 3, {41, 35, 39, 255});
            tilePixel(x, y, pixel, 10, 9, 2, 1, {105, 80, 65, 190});
        }
        if (maze.isWalkable(row + 1, column))
        {
            tilePixel(x, y, pixel, 0, 12, 16, 4, {20, 20, 27, 255});
            tilePixel(x, y, pixel, 1, 12, 14, 1, {67, 48, 52, 255});
            tilePixel(x, y, pixel, 3, 13, 1, 2, {36, 28, 35, 255});
        }
        if (hash % 4U == 0U)
        {
            unsigned char glow = static_cast<unsigned char>(190.0f + 35.0f * std::sin(time * 1.4f + static_cast<float>(hash % 17U)));
            tilePixel(x, y, pixel, 5, 2, 4, 10, {229, 70, 22, 30});
            tilePixel(x, y, pixel, 7, 2, 2, 4, {38, 24, 28, 255});
            tilePixel(x, y, pixel, 6, 6, 2, 4, {38, 24, 28, 255});
            tilePixel(x, y, pixel, 8, 9, 4, 2, {38, 24, 28, 255});
            tilePixel(x, y, pixel, 7, 3, 1, 3, {255, 118, 37, glow});
            tilePixel(x, y, pixel, 6, 6, 1, 3, {246, 81, 26, glow});
            tilePixel(x, y, pixel, 7, 9, 4, 1, {255, 143, 47, glow});
            tilePixel(x, y, pixel, 7, 4, 1, 1, {255, 215, 112, glow});
        }
        if (maze.getCell(row, column) == '?')
        {
            tilePixel(x, y, pixel, 9, 3, 1, 3, {69, 117, 112, 230});
            tilePixel(x, y, pixel, 8, 6, 1, 3, {104, 144, 121, 230});
            tilePixel(x, y, pixel, 9, 9, 1, 2, {190, 138, 67, 230});
        }
    }

    void drawBoulder(float x, float y, int tileSize)
    {
        float pixel = static_cast<float>(tileSize) / 16.0f;
        tilePixel(x, y, pixel, 2, 12, 13, 3, {19, 20, 25, 160});
        tilePixel(x, y, pixel, 3, 5, 10, 9, {65, 55, 59, 255});
        tilePixel(x, y, pixel, 5, 3, 6, 2, {86, 70, 68, 255});
        tilePixel(x, y, pixel, 2, 7, 2, 5, {59, 49, 55, 255});
        tilePixel(x, y, pixel, 5, 5, 6, 2, {109, 84, 71, 255});
        tilePixel(x, y, pixel, 11, 6, 3, 7, {45, 40, 48, 255});
        tilePixel(x, y, pixel, 7, 8, 1, 4, {36, 33, 41, 255});
        tilePixel(x, y, pixel, 8, 11, 3, 1, {36, 33, 41, 255});
    }

    void drawMarker(float x, float y, int tileSize, bool exit, float time)
    {
        float pixel = static_cast<float>(tileSize) / 16.0f;
        if (exit)
        {
            unsigned char pulse = static_cast<unsigned char>(150.0f + 35.0f * std::sin(time * 2.2f));
            tilePixel(x, y, pixel, 2, 1, 12, 14, {234, 153, 58, 26});
            tilePixel(x, y, pixel, 3, 3, 2, 11, {162, 112, 69, 255});
            tilePixel(x, y, pixel, 11, 3, 2, 11, {113, 76, 56, 255});
            tilePixel(x, y, pixel, 5, 1, 6, 3, {227, 182, 108, 255});
            tilePixel(x, y, pixel, 5, 4, 6, 9, {43, 107, 98, pulse});
            tilePixel(x, y, pixel, 6, 4, 1, 8, {116, 229, 187, pulse});
            tilePixel(x, y, pixel, 9, 5, 1, 7, {244, 190, 83, pulse});
            tilePixel(x, y, pixel, 2, 14, 12, 1, {235, 181, 102, 255});
            tilePixel(x, y, pixel, 7, 2, 2, 1, {255, 237, 177, 255});
        }
        else
        {
            tilePixel(x, y, pixel, 3, 3, 10, 10, {39, 75, 74, 255});
            tilePixel(x, y, pixel, 4, 4, 8, 8, {40, 46, 47, 255});
            tilePixel(x, y, pixel, 7, 5, 2, 6, {82, 165, 147, 220});
            tilePixel(x, y, pixel, 5, 7, 6, 2, {82, 165, 147, 220});
            tilePixel(x, y, pixel, 7, 7, 2, 2, {181, 201, 155, 255});
        }
    }

    void drawExplorer(float x, float y, int tileSize, const GameVisualState& state)
    {
        float pixel = static_cast<float>(tileSize) / 16.0f;
        float swing = state.moving ? std::sin(state.walkPhase) : 0.0f;
        float bob = state.moving ? (std::abs(swing) > 0.5f ? -1.0f : 0.0f)
            : (std::sin(state.idleTime * 2.2f) > 0.8f ? -0.4f : 0.0f);
        int leftStep = swing > 0.3f ? 1 : 0;
        int rightStep = swing < -0.3f ? 1 : 0;
        tilePixel(x, y, pixel, 3, 14, 11, 2, {10, 13, 19, 150});
        y += bob * pixel;
        Color outline = {18, 22, 29, 255};
        tilePixel(x, y, pixel, 4, 8, 8, 6, outline);
        tilePixel(x, y, pixel, 4, 12 + static_cast<float>(leftStep), 3, 3, outline);
        tilePixel(x, y, pixel, 9, 12 + static_cast<float>(rightStep), 3, 3, outline);
        tilePixel(x, y, pixel, 4, 13 + static_cast<float>(leftStep), 3, 1, {117, 77, 48, 255});
        tilePixel(x, y, pixel, 9, 13 + static_cast<float>(rightStep), 3, 1, {117, 77, 48, 255});
        tilePixel(x, y, pixel, 5, 9, 6, 4, {169, 115, 55, 255});
        tilePixel(x, y, pixel, 3, 9 - swing, 2, 3, outline);
        tilePixel(x, y, pixel, 11, 9 + swing, 2, 3, outline);
        tilePixel(x, y, pixel, 3, 10 - swing, 1, 2, {217, 165, 108, 255});
        tilePixel(x, y, pixel, 12, 10 + swing, 1, 2, {217, 165, 108, 255});
        tilePixel(x, y, pixel, 4, 2, 8, 6, outline);
        tilePixel(x, y, pixel, 5, 1, 6, 1, outline);
        tilePixel(x, y, pixel, 3, 4, 10, 3, outline);
        tilePixel(x, y, pixel, 5, 2, 6, 1, {248, 224, 158, 255});
        tilePixel(x, y, pixel, 4, 3, 8, 3, {221, 185, 116, 255});
        tilePixel(x, y, pixel, 3, 6, 10, 1, {255, 222, 146, 255});
        tilePixel(x, y, pixel, 5, 3, 4, 1, {255, 237, 184, 255});
        if (state.facing == 0)
        {
            tilePixel(x, y, pixel, 5, 7, 6, 2, {84, 65, 46, 255});
            tilePixel(x, y, pixel, 5, 9, 6, 4, {65, 92, 86, 255});
            tilePixel(x, y, pixel, 6, 9, 4, 3, {100, 126, 101, 255});
            tilePixel(x, y, pixel, 6, 10, 4, 1, {183, 159, 99, 255});
        }
        else
        {
            bool side = state.facing == 2 || state.facing == 3;
            float faceX = state.facing == 2 ? 4.0f : (state.facing == 3 ? 8.0f : 5.0f);
            tilePixel(x, y, pixel, faceX, 7, side ? 4.0f : 6.0f, 2, {225, 173, 112, 255});
            tilePixel(x, y, pixel, side ? (state.facing == 2 ? 4.0f : 10.0f) : 6.0f, 7, 1, 1, outline);
            if (!side) { tilePixel(x, y, pixel, 9, 7, 1, 1, outline); }
            tilePixel(x, y, pixel, 5, 9, 6, 1, {62, 173, 155, 255});
            tilePixel(x, y, pixel, state.facing == 2 ? 10.0f : 5.0f, 10, 2, 2, {42, 135, 132, 255});
            float lampX = state.facing == 2 ? 3.0f : (state.facing == 3 ? 11.0f : 7.0f);
            tilePixel(x, y, pixel, lampX, 4, 2, 2, {84, 75, 53, 255});
            tilePixel(x, y, pixel, lampX, 4, 1, 1, {255, 250, 209, 255});
        }
    }
}

GameViewLayout createGameViewLayout(int screenWidth, int screenHeight,
    const GameLogic& game, const GameVisualState& visualState)
{
    GameViewLayout layout = {};
    layout.scale = std::max(0.001f, std::min(static_cast<float>(screenWidth) / 1600.0f,
        static_cast<float>(screenHeight) / 900.0f));
    layout.viewport = {std::floor((static_cast<float>(screenWidth) - 1600.0f * layout.scale) * 0.5f),
        std::floor((static_cast<float>(screenHeight) - 900.0f * layout.scale) * 0.5f),
        1600.0f * layout.scale, 900.0f * layout.scale};
    layout.hud = scaledRectangle(layout, 32, 28, 1536, 112);
    layout.world = scaledRectangle(layout, 34, 182, 1170, 660);
    layout.map = scaledRectangle(layout, 1250, 220, 296, 296);
    int pixelScale = std::max(1, static_cast<int>(std::round(2.5f * layout.scale)));
    layout.tileSize = 16 * pixelScale;
    float mapWidth = static_cast<float>(game.getMaze().getColumnCount() * layout.tileSize);
    float mapHeight = static_cast<float>(game.getMaze().getRowCount() * layout.tileSize);
    float playerX = (visualState.playerColumn + 0.5f) * static_cast<float>(layout.tileSize);
    float playerY = (visualState.playerRow + 0.5f) * static_cast<float>(layout.tileSize);
    layout.cameraX = std::round(mapWidth <= layout.world.width ? (mapWidth - layout.world.width) * 0.5f
        : std::clamp(playerX - layout.world.width * 0.5f, 0.0f, mapWidth - layout.world.width));
    layout.cameraY = std::round(mapHeight <= layout.world.height ? (mapHeight - layout.world.height) * 0.5f
        : std::clamp(playerY - layout.world.height * 0.5f, 0.0f, mapHeight - layout.world.height));
    return layout;
}

GameArt::GameArt() : titleFont(GetFontDefault()), bodyFont(GetFontDefault())
{
    const std::string assetRoot = std::string(GetApplicationDirectory()) + "assets/fonts/";
    int codepoints[95] = {};
    for (int codepointIndex = 0; codepointIndex < 95; codepointIndex++)
    {
        codepoints[codepointIndex] = codepointIndex + 32;
    }
    std::string titlePath = assetRoot + "PressStart2P-Regular.ttf";
    if (FileExists(titlePath.c_str()))
    {
        titleFont = LoadFontEx(titlePath.c_str(), 64, codepoints, 95);
        ownsTitleFont = titleFont.texture.id != 0 && titleFont.texture.id != GetFontDefault().texture.id;
        if (ownsTitleFont) { SetTextureFilter(titleFont.texture, TEXTURE_FILTER_POINT); }
        else { titleFont = GetFontDefault(); }
    }
    std::string bodyPath = assetRoot + "Rajdhani-Medium.ttf";
    if (FileExists(bodyPath.c_str()))
    {
        bodyFont = LoadFontEx(bodyPath.c_str(), 64, codepoints, 95);
        ownsBodyFont = bodyFont.texture.id != 0 && bodyFont.texture.id != GetFontDefault().texture.id;
        if (ownsBodyFont) { SetTextureFilter(bodyFont.texture, TEXTURE_FILTER_BILINEAR); }
        else { bodyFont = GetFontDefault(); }
    }
}

GameArt::~GameArt()
{
    if (ownsTitleFont) { UnloadFont(titleFont); }
    if (ownsBodyFont) { UnloadFont(bodyFont); }
}

void GameArt::label(const GameViewLayout& layout, const char* text, float x, float y,
    float size, Color color, bool pixelFont) const
{
    Rectangle position = scaledRectangle(layout, x, y, 0, 0);
    float fontSize = pixelFont ? std::max(8.0f, std::round(size * layout.scale / 8.0f) * 8.0f) : size * layout.scale;
    DrawTextEx(pixelFont ? titleFont : bodyFont, text, {position.x, position.y},
        fontSize, (pixelFont ? 1.0f : 0.5f) * layout.scale, color);
}

void GameArt::centered(const GameViewLayout& layout, const char* text, float centerX,
    float y, float size, Color color, bool pixelFont) const
{
    Font font = pixelFont ? titleFont : bodyFont;
    float spacing = (pixelFont ? 1.0f : 0.5f) * layout.scale;
    float fontSize = pixelFont ? std::max(8.0f, std::round(size * layout.scale / 8.0f) * 8.0f) : size * layout.scale;
    Vector2 extent = MeasureTextEx(font, text, fontSize, spacing);
    Rectangle position = scaledRectangle(layout, centerX, y, 0, 0);
    DrawTextEx(font, text, {std::round(position.x - extent.x * 0.5f), position.y},
        fontSize, spacing, color);
}

void GameArt::drawHud(const GameLogic& game, const GameViewLayout& layout) const
{
    panel(layout, 32, 28, 1536, 112);
    Color cream = {241, 222, 187, 255};
    Color muted = {158, 145, 132, 255};
    Color amber = {255, 188, 87, 255};
    label(layout, "WOLCAN", 54, 49, 20, amber, true);
    label(layout, TextFormat("EXPEDITION  %02i", game.getLevelNumber()), 54, 89, 19, cream);
    const int dividerCount = 6;
    const float dividers[dividerCount] = {265, 458, 652, 822, 1048, 1274};
    for (int dividerIndex = 0; dividerIndex < dividerCount; dividerIndex++)
    {
        DrawRectangleRec(scaledRectangle(layout, dividers[dividerIndex], 50, 1, 66), {80, 62, 52, 255});
    }
    label(layout, "DIFFICULTY", 289, 48, 20, muted);
    label(layout, difficultyName(game.getMaze().getDifficulty()), 289, 76, 34, cream);
    label(layout, "POSITION  /  ROW : COL", 482, 48, 20, muted);
    label(layout, TextFormat("%02i : %02i", game.getPlayer().getRow() + 1, game.getPlayer().getColumn() + 1),
        482, 76, 34, cream);
    label(layout, "MOVES", 676, 48, 20, muted);
    label(layout, TextFormat("%i", game.getMoveCount()), 676, 76, 34, cream);
    char elapsed[32] = {};
    char best[32] = {};
    formatTime(game.getElapsedSeconds(), elapsed, sizeof(elapsed));
    formatTime(game.getBestSeconds(), best, sizeof(best));
    label(layout, "ELAPSED", 846, 48, 20, muted);
    label(layout, elapsed, 846, 76, 34, amber);
    label(layout, "BEST  /  THIS SEED", 1072, 48, 20, muted);
    label(layout, best, 1072, 76, 34, cream);
    label(layout, TextFormat("SEED  %u", static_cast<unsigned int>(game.getMaze().getSeed())), 1298, 48, 22, muted);
    label(layout, TextFormat("SECRETS  %i / %i", game.getSecretsFound(), game.getMaze().getFalseWallCount()),
        1298, 83, 21, {114, 192, 170, 255});
}

void GameArt::drawMap(const GameLogic& game, const GameVisualState& state, const GameViewLayout& layout) const
{
    panel(layout, 1230, 180, 338, 662);
    Color cream = {233, 215, 183, 255};
    Color muted = {157, 145, 132, 255};
    label(layout, "ISLAND SURVEY", 1250, 190, 22, cream);
    const Maze& maze = game.getMaze();
    float pixel = std::floor(std::min(layout.map.width / static_cast<float>(maze.getColumnCount()),
        layout.map.height / static_cast<float>(maze.getRowCount())));
    pixel = std::max(1.0f, pixel);
    float mapWidth = pixel * static_cast<float>(maze.getColumnCount());
    float mapHeight = pixel * static_cast<float>(maze.getRowCount());
    float left = std::round(layout.map.x + (layout.map.width - mapWidth) * 0.5f);
    float top = std::round(layout.map.y + (layout.map.height - mapHeight) * 0.5f);
    DrawRectangleRec(layout.map, {15, 17, 23, 255});
    for (int row = 0; row < maze.getRowCount(); row++)
    {
        for (int column = 0; column < maze.getColumnCount(); column++)
        {
            char cell = maze.getCell(row, column);
            Color color = {75, 66, 64, 255};
            if (cell == '#' || cell == '?') { color = {34, 35, 45, 255}; }
            if (cell == 'X') { color = {48, 44, 51, 255}; }
            if (cell == ',' || cell == '+') { color = {105, 132, 113, 255}; }
            DrawRectangleRec({left + static_cast<float>(column) * pixel, top + static_cast<float>(row) * pixel,
                pixel, pixel}, color);
        }
    }
    GridPosition start = maze.getStart();
    GridPosition exit = maze.getExit();
    DrawRectangleRec({left + static_cast<float>(start.column) * pixel, top + static_cast<float>(start.row) * pixel,
        pixel, pixel}, {81, 170, 153, 255});
    DrawRectangleRec({left + static_cast<float>(exit.column) * pixel, top + static_cast<float>(exit.row) * pixel,
        pixel, pixel}, {245, 184, 79, 255});
    float heroX = left + (state.playerColumn + 0.5f) * pixel;
    float heroY = top + (state.playerRow + 0.5f) * pixel;
    float markerSize = std::max(3.0f, pixel);
    DrawRectangleRec({std::round(heroX - markerSize * 0.5f - 1), std::round(heroY - markerSize * 0.5f - 1),
        markerSize + 2, markerSize + 2}, {20, 23, 28, 255});
    DrawRectangleRec({std::round(heroX - markerSize * 0.5f), std::round(heroY - markerSize * 0.5f),
        markerSize, markerSize}, {249, 236, 196, 255});
    DrawRectangleRec(scaledRectangle(layout, 1250, 534, 8, 8), {249, 236, 196, 255});
    label(layout, "YOU", 1265, 527, 20, muted);
    DrawRectangleRec(scaledRectangle(layout, 1345, 534, 8, 8), {105, 132, 113, 255});
    label(layout, "TRAIL", 1360, 527, 20, muted);
    DrawRectangleRec(scaledRectangle(layout, 1453, 534, 8, 8), {245, 184, 79, 255});
    label(layout, "EXIT", 1468, 527, 20, muted);
    DrawRectangleRec(scaledRectangle(layout, 1250, 565, 296, 1), {76, 60, 51, 255});
    label(layout, "FIND THE SUNLIT GATE", 1250, 583, 18, {255, 188, 87, 255});
    label(layout, "Mineral seams hide paths.", 1250, 612, 22, muted);
    label(layout, "Walk into cracked walls.", 1250, 635, 22, muted);
    DrawRectangleRec(scaledRectangle(layout, 1250, 674, 296, 1), {76, 60, 51, 255});
    label(layout, "ARROWS", 1250, 691, 22, cream);
    label(layout, "Move", 1390, 691, 22, muted);
    label(layout, "P", 1250, 720, 22, cream);
    label(layout, "Pause / resume", 1390, 720, 22, muted);
    label(layout, "R  /  T", 1250, 749, 22, cream);
    label(layout, "New / same seed", 1390, 749, 22, muted);
    label(layout, "ESC", 1250, 778, 22, cream);
    label(layout, "Main menu", 1390, 778, 22, muted);
    label(layout, "N", 1250, 807, 22, cream);
    label(layout, "Next after escape", 1390, 807, 22, muted);
}

void GameArt::drawOverlay(const GameLogic& game, const GameViewLayout& layout) const
{
    if (!game.isPaused() && !game.hasWon()) { return; }
    DrawRectangleRec(layout.world, {8, 10, 17, 170});
    bool won = game.hasWon();
    float height = won ? 346.0f : 254.0f;
    float top = won ? 326.0f : 370.0f;
    panel(layout, 303, top, 632, height);
    DrawRectangleRec(scaledRectangle(layout, 553, top, 132, 3), {255, 188, 87, 255});
    centered(layout, won ? "ISLAND ESCAPED" : "PAUSED", 619, top + 35, won ? 26.0f : 30.0f,
        {255, 194, 101, 255}, true);
    centered(layout, won ? "The gate is open. Your next expedition awaits." : "Catch your breath. The clock is stopped.",
        619, top + 93, 23, {224, 206, 181, 255});
    if (won)
    {
        char elapsed[32] = {};
        char best[32] = {};
        formatTime(game.getElapsedSeconds(), elapsed, sizeof(elapsed));
        formatTime(game.getBestSeconds(), best, sizeof(best));
        label(layout, "FINAL TIME", 351, top + 141, 17, {161, 147, 132, 255});
        label(layout, elapsed, 351, top + 169, 35, {248, 226, 183, 255});
        label(layout, "SEED BEST", 668, top + 141, 17, {161, 147, 132, 255});
        label(layout, best, 668, top + 169, 35, {120, 201, 173, 255});
        centered(layout, game.wasRecordSaved() ? "BEST TIME SAVED" : "RECORD COULD NOT BE SAVED",
            619, top + 215, 16, game.wasRecordSaved() ? Color{120, 201, 173, 255} : Color{235, 158, 104, 255});
        centered(layout, "N   NEXT EXPEDITION", 619, top + 249, 24, {255, 194, 101, 255});
    }
    else
    {
        centered(layout, "P   RESUME EXPEDITION", 619, top + 148, 24, {120, 201, 173, 255});
    }
    centered(layout, "R  NEW SEED     T  REPLAY     ESC  MENU", 619, top + height - 43, 19,
        {190, 170, 145, 255});
}

void GameArt::draw(const GameLogic& game, const GameVisualState& visualState) const
{
    GameViewLayout layout = createGameViewLayout(GetScreenWidth(), GetScreenHeight(), game, visualState);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {7, 8, 12, 255});
    DrawRectangleRec(layout.viewport, {17, 17, 22, 255});
    DrawRectangleRec(scaledRectangle(layout, 0, 0, 1600, 5), {112, 53, 34, 255});
    drawHud(game, layout);
    label(layout, "THE OBSIDIAN LABYRINTH", 34, 154, 16, {182, 160, 133, 255});
    label(layout, TextFormat("%02i x %02i", game.getMaze().getRowCount(), game.getMaze().getColumnCount()),
        1114, 154, 16, {137, 128, 122, 255});
    panel(layout, 32, 180, 1174, 664);
    DrawRectangleRec(layout.world, {13, 16, 22, 255});
    BeginScissorMode(static_cast<int>(layout.world.x), static_cast<int>(layout.world.y),
        static_cast<int>(layout.world.width), static_cast<int>(layout.world.height));
    const Maze& maze = game.getMaze();
    int firstRow = std::max(0, static_cast<int>(std::floor(layout.cameraY / static_cast<float>(layout.tileSize))));
    int firstColumn = std::max(0, static_cast<int>(std::floor(layout.cameraX / static_cast<float>(layout.tileSize))));
    int lastRow = std::min(maze.getRowCount(), static_cast<int>(std::ceil((layout.cameraY + layout.world.height)
        / static_cast<float>(layout.tileSize))) + 1);
    int lastColumn = std::min(maze.getColumnCount(), static_cast<int>(std::ceil((layout.cameraX + layout.world.width)
        / static_cast<float>(layout.tileSize))) + 1);
    int visibleRows = lastRow - firstRow;
    int visibleColumns = lastColumn - firstColumn;
    for (int rowIndex = 0; rowIndex < visibleRows; rowIndex++)
    {
        int row = firstRow + rowIndex;
        for (int columnIndex = 0; columnIndex < visibleColumns; columnIndex++)
        {
            int column = firstColumn + columnIndex;
            float x = layout.world.x + static_cast<float>(column * layout.tileSize) - layout.cameraX;
            float y = layout.world.y + static_cast<float>(row * layout.tileSize) - layout.cameraY;
            char cell = maze.getCell(row, column);
            if (cell == '#' || cell == '?') { drawWall(maze, row, column, x, y, layout.tileSize, visualState.idleTime); }
            else
            {
                drawFloor(x, y, layout.tileSize, tileHash(row, column), cell);
                if (cell == 'X') { drawBoulder(x, y, layout.tileSize); }
            }
        }
    }
    GridPosition start = maze.getStart();
    GridPosition exit = maze.getExit();
    drawMarker(layout.world.x + static_cast<float>(start.column * layout.tileSize) - layout.cameraX,
        layout.world.y + static_cast<float>(start.row * layout.tileSize) - layout.cameraY, layout.tileSize, false, visualState.idleTime);
    drawMarker(layout.world.x + static_cast<float>(exit.column * layout.tileSize) - layout.cameraX,
        layout.world.y + static_cast<float>(exit.row * layout.tileSize) - layout.cameraY, layout.tileSize, true, visualState.idleTime);
    drawExplorer(std::round(layout.world.x + visualState.playerColumn * static_cast<float>(layout.tileSize) - layout.cameraX),
        std::round(layout.world.y + visualState.playerRow * static_cast<float>(layout.tileSize) - layout.cameraY), layout.tileSize, visualState);
    const int emberCount = 15;
    for (int emberIndex = 0; emberIndex < emberCount; emberIndex++)
    {
        float index = static_cast<float>(emberIndex);
        float x = layout.world.x + std::fmod(index * 149.0f * layout.scale + 37.0f * layout.scale, layout.world.width);
        float y = layout.world.y + layout.world.height - std::fmod(index * 67.0f * layout.scale
            + visualState.idleTime * (6.0f + static_cast<float>(emberIndex % 5)) * layout.scale, layout.world.height);
        float size = std::max(1.0f, std::round(layout.scale));
        DrawRectangleRec({std::round(x), std::round(y), size, size}, {240, 138, 63, 110});
    }
    EndScissorMode();
    drawMap(game, visualState, layout);
    label(layout, "VOLCANIC ISLE  /  FIELD NOTES", 34, 863, 16, {125, 116, 108, 255});
    label(layout, "F11  FULLSCREEN", 1408, 863, 16, {153, 136, 115, 255});
    drawOverlay(game, layout);
}
