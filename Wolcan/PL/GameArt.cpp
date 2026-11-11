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
        int hundredths = static_cast<int>(std::lround(bounded * 100.0));
        std::snprintf(destination, capacity, "%02i:%02i.%02i",
            hundredths / 6000, hundredths / 100 % 60, hundredths % 100);
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

    Color withOpacity(Color color, float opacity)
    {
        color.a = static_cast<unsigned char>(std::round(static_cast<float>(color.a)
            * std::clamp(opacity, 0.0f, 1.0f)));
        return color;
    }

    void drawFloor(float x, float y, int tileSize, unsigned int hash)
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
    }

    void drawWall(const Maze& maze, int row, int column, float x, float y, int tileSize, float time)
    {
        unsigned int hash = tileHash(row, column);
        float pixel = static_cast<float>(tileSize) / 16.0f;
        unsigned char shade = static_cast<unsigned char>(hash % 9U);
        drawFloor(x, y, tileSize, hash);
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

    void drawFragileFloor(float x, float y, float pixel, const CellFeature& feature)
    {
        float crack = feature.phase == HazardPhase::Warning ? feature.phaseProgress : 0.0f;
        Color seam = {29, 19, 23, 255};
        Color glow = withOpacity({255, 124, 48, 255}, 0.25f + 0.75f * crack);
        tilePixel(x, y, pixel, 1, 1, 14, 14, {43, 26, 29, 255});
        tilePixel(x, y, pixel, 2, 2, 12, 11, {98, 60, 47, 255});
        tilePixel(x, y, pixel, 2, 2, 12, 1, {145, 94, 65, 255});
        tilePixel(x, y, pixel, 3, 13, 10, 1, glow);
        tilePixel(x, y, pixel, 5, 2, 2, 4, seam);
        tilePixel(x, y, pixel, 6, 5, 4, 2, seam);
        tilePixel(x, y, pixel, 8, 7, 2, 3, seam);
        tilePixel(x, y, pixel, 9, 9, 4, 2, seam);
        tilePixel(x, y, pixel, 12, 10, 2, 3, seam);
        tilePixel(x, y, pixel, 7, 6, 2, 1, glow);
        tilePixel(x, y, pixel, 9, 7, 1, 3, glow);
        tilePixel(x, y, pixel, 10, 10, 3, 1, glow);
        if (crack > 0.0f)
        {
            float spread = std::floor(crack * 3.0f);
            tilePixel(x, y, pixel, 3, 7 + spread, 4, 1, seam);
            tilePixel(x, y, pixel, 4 + spread, 8, 2, 4, glow);
            tilePixel(x, y, pixel, 11, 4 + spread, 2, 1, glow);
            tilePixel(x, y, pixel, 2, 2, 12, 11, withOpacity({255, 88, 42, 64}, crack));
        }
    }

    void drawLava(float x, float y, float pixel, unsigned int hash, float time)
    {
        tilePixel(x, y, pixel, 0, 0, 16, 16, {65, 28, 29, 255});
        tilePixel(x, y, pixel, 1, 1, 14, 14, {145, 42, 28, 255});
        tilePixel(x, y, pixel, 2, 2, 12, 12, {216, 66, 26, 255});
        tilePixel(x, y, pixel, 3, 3, 10, 10, {239, 91, 28, 255});
        for (int lane = 0; lane < 4; lane++)
        {
            float wave = std::fmod(time * (5.0f + static_cast<float>(lane))
                + static_cast<float>(hash % 13U) + 4.0f * static_cast<float>(lane), 11.0f);
            float offset = std::floor(wave);
            float lineY = 3.0f + static_cast<float>(lane) * 3.0f;
            tilePixel(x, y, pixel, 2 + offset, lineY, std::min(5.0f, 12.0f - offset), 1,
                {255, 164, 45, 255});
            if (offset < 10.0f)
            {
                tilePixel(x, y, pixel, 3 + offset, lineY, std::min(2.0f, 11.0f - offset), 1,
                    {255, 220, 112, 255});
            }
        }
        tilePixel(x, y, pixel, 1, 2, 1, 4, {83, 38, 32, 255});
        tilePixel(x, y, pixel, 13, 13, 2, 2, {83, 38, 32, 255});
    }

    void drawWallSpikes(float x, float y, float pixel, const CellFeature& feature)
    {
        float row = static_cast<float>(feature.wallRowOffset);
        float column = static_cast<float>(feature.wallColumnOffset);
        auto spikePixel = [=](float across, float depth, float width, Color color)
        {
            if (row != 0.0f)
            {
                float top = row < 0.0f ? depth : 15.0f - depth;
                tilePixel(x, y, pixel, across, top, width, 1, color);
            }
            else
            {
                float left = column < 0.0f ? depth : 15.0f - depth;
                tilePixel(x, y, pixel, left, across, 1, width, color);
            }
        };
        spikePixel(2, 0, 12, {35, 26, 31, 255});
        spikePixel(3, 1, 10, {101, 71, 63, 255});
        if (feature.phase == HazardPhase::Dormant) { return; }
        if (feature.phase == HazardPhase::Warning)
        {
            float flash = 0.45f + 0.55f * std::abs(std::sin(feature.phaseProgress * 12.0f));
            spikePixel(3, 1, 10, withOpacity({255, 139, 52, 255}, flash));
            spikePixel(5, 3, 6, withOpacity({240, 87, 37, 170}, flash));
            spikePixel(7, 5, 2, withOpacity({255, 193, 91, 255}, flash));
            return;
        }

        float extension = feature.phase == HazardPhase::Active
            ? std::min(1.0f, 0.6f + 5.0f * feature.phaseProgress) : 1.0f - feature.phaseProgress;
        int length = static_cast<int>(std::ceil(12.0f * extension));
        for (int depthIndex = 0; depthIndex < length; depthIndex++)
        {
            float depth = static_cast<float>(depthIndex);
            float inset = depthIndex >= 7 ? 1.0f : 0.0f;
            float width = 3.0f - inset * 2.0f;
            for (int spikeIndex = 0; spikeIndex < 3; spikeIndex++)
            {
                float across = 3.0f + 4.0f * static_cast<float>(spikeIndex) + inset;
                spikePixel(across, depth, width, {169, 128, 104, 255});
                spikePixel(across, depth, 1, {238, 199, 144, 255});
            }
        }
    }

    void drawItem(float x, float y, float pixel, ItemKind kind, float glow)
    {
        Color shade = withOpacity({36, 24, 40, 255}, glow);
        if (kind == ItemKind::Rune)
        {
            Color stone = withOpacity({166, 146, 213, 255}, glow);
            Color script = withOpacity({251, 226, 168, 255}, glow);
            tilePixel(x, y, pixel, 4, 4, 8, 9, shade);
            tilePixel(x, y, pixel, 5, 5, 6, 7, stone);
            tilePixel(x, y, pixel, 7, 6, 1, 5, script);
            tilePixel(x, y, pixel, 8, 6, 2, 1, script);
            tilePixel(x, y, pixel, 8, 8, 2, 1, script);
        }
        else if (kind == ItemKind::Crystal)
        {
            tilePixel(x, y, pixel, 7, 3, 3, 10, shade);
            tilePixel(x, y, pixel, 5, 5, 7, 6, shade);
            tilePixel(x, y, pixel, 7, 4, 2, 8, withOpacity({255, 199, 117, 255}, glow));
            tilePixel(x, y, pixel, 6, 6, 5, 4, withOpacity({244, 98, 69, 255}, glow));
            tilePixel(x, y, pixel, 7, 5, 1, 4, withOpacity({255, 238, 188, 255}, glow));
        }
        else if (kind == ItemKind::Relic)
        {
            Color gold = withOpacity({236, 172, 84, 255}, glow);
            tilePixel(x, y, pixel, 4, 5, 8, 7, shade);
            tilePixel(x, y, pixel, 4, 5, 8, 2, gold);
            tilePixel(x, y, pixel, 6, 7, 4, 3, gold);
            tilePixel(x, y, pixel, 5, 11, 6, 1, gold);
            tilePixel(x, y, pixel, 7, 6, 2, 3, withOpacity({255, 231, 157, 255}, glow));
        }
    }

    void drawPickup(float x, float y, float pixel, float flash)
    {
        float radius = 2.0f + (1.0f - flash) * 9.0f;
        Color spark = withOpacity({255, 225, 138, 255}, flash);
        for (int sparkIndex = 0; sparkIndex < 8; sparkIndex++)
        {
            float angle = static_cast<float>(sparkIndex) * 0.7853982f;
            float sparkX = 8.0f + radius * std::cos(angle);
            float sparkY = 8.0f + radius * std::sin(angle);
            tilePixel(x, y, pixel, sparkX, sparkY, 1, 1, spark);
        }
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

    void drawExplorer(float x, float y, int tileSize, const GameVisualState& state,
        Texture2D texture, float deathProgress, bool inLava)
    {
        float pixel = static_cast<float>(tileSize) / 16.0f;
        float swing = state.moving ? std::sin(state.walkPhase) : 0.0f;
        float bob = state.moving ? (std::abs(swing) > 0.5f ? -1.0f : 0.0f)
            : (std::sin(state.idleTime * 2.2f) > 0.8f ? -0.4f : 0.0f);
        float opacity = 1.0f - deathProgress;
        float tint = std::max(state.damageFlash, deathProgress);
        Color hitColor = {255, static_cast<unsigned char>(255.0f - tint * 132.0f),
            static_cast<unsigned char>(255.0f - tint * 158.0f), 255};
        tilePixel(x, y, pixel, 3, 14, 11, 2, withOpacity({10, 13, 19, 150}, opacity));
        y += std::floor(deathProgress * 5.0f + (inLava ? 1.0f : 0.0f)) * pixel;
        if (texture.id != 0)
        {
            // Columns are idle and two strides. Rows are up, down, left and right.
            const float frameSize = 16.0f;
            int frame = swing > 0.5f ? 1 : (swing < -0.5f ? 2 : 0);
            float direction = static_cast<float>(std::clamp(state.facing, 0, 3));
            float spriteY = state.moving ? y : std::round(y + bob * pixel);
            float size = static_cast<float>(tileSize);
            DrawTexturePro(texture, {static_cast<float>(frame) * frameSize, direction * frameSize, frameSize, frameSize},
                {x, spriteY, size, size}, {0, 0}, 0, withOpacity(hitColor, opacity));
            return;
        }

        // Preserve the code-drawn explorer if the sprite sheet is unavailable.
        int leftStep = swing > 0.3f ? 1 : 0;
        int rightStep = swing < -0.3f ? 1 : 0;
        y += bob * pixel;
        auto explorerPixel = [=](float left, float top, float width, float height, Color color)
        {
            color.g = static_cast<unsigned char>(static_cast<int>(color.g) * hitColor.g / 255);
            color.b = static_cast<unsigned char>(static_cast<int>(color.b) * hitColor.b / 255);
            tilePixel(x, y, pixel, left, top, width, height, withOpacity(color, opacity));
        };
        Color outline = {18, 22, 29, 255};
        explorerPixel(4, 8, 8, 6, outline);
        explorerPixel(4, 12 + static_cast<float>(leftStep), 3, 3, outline);
        explorerPixel(9, 12 + static_cast<float>(rightStep), 3, 3, outline);
        explorerPixel(4, 13 + static_cast<float>(leftStep), 3, 1, {117, 77, 48, 255});
        explorerPixel(9, 13 + static_cast<float>(rightStep), 3, 1, {117, 77, 48, 255});
        explorerPixel(5, 9, 6, 4, {169, 115, 55, 255});
        explorerPixel(3, 9 - swing, 2, 3, outline);
        explorerPixel(11, 9 + swing, 2, 3, outline);
        explorerPixel(3, 10 - swing, 1, 2, {217, 165, 108, 255});
        explorerPixel(12, 10 + swing, 1, 2, {217, 165, 108, 255});
        explorerPixel(4, 2, 8, 6, outline);
        explorerPixel(5, 1, 6, 1, outline);
        explorerPixel(3, 4, 10, 3, outline);
        explorerPixel(5, 2, 6, 1, {248, 224, 158, 255});
        explorerPixel(4, 3, 8, 3, {221, 185, 116, 255});
        explorerPixel(3, 6, 10, 1, {255, 222, 146, 255});
        explorerPixel(5, 3, 4, 1, {255, 237, 184, 255});
        if (state.facing == 0)
        {
            explorerPixel(5, 7, 6, 2, {84, 65, 46, 255});
            explorerPixel(5, 9, 6, 4, {65, 92, 86, 255});
            explorerPixel(6, 9, 4, 3, {100, 126, 101, 255});
            explorerPixel(6, 10, 4, 1, {183, 159, 99, 255});
        }
        else
        {
            bool side = state.facing == 2 || state.facing == 3;
            float faceX = state.facing == 2 ? 4.0f : (state.facing == 3 ? 8.0f : 5.0f);
            explorerPixel(faceX, 7, side ? 4.0f : 6.0f, 2, {225, 173, 112, 255});
            explorerPixel(side ? (state.facing == 2 ? 4.0f : 10.0f) : 6.0f, 7, 1, 1, outline);
            if (!side) { explorerPixel(9, 7, 1, 1, outline); }
            explorerPixel(5, 9, 6, 1, {62, 173, 155, 255});
            explorerPixel(state.facing == 2 ? 10.0f : 5.0f, 10, 2, 2, {42, 135, 132, 255});
            float lampX = state.facing == 2 ? 3.0f : (state.facing == 3 ? 11.0f : 7.0f);
            explorerPixel(lampX, 4, 2, 2, {84, 75, 53, 255});
            explorerPixel(lampX, 4, 1, 1, {255, 250, 209, 255});
        }
    }

    void drawDeath(float x, float y, float pixel, float progress)
    {
        for (int fragment = 0; fragment < 10; fragment++)
        {
            unsigned int hash = tileHash(fragment, 0);
            float direction = static_cast<float>(hash % 11U) - 5.0f;
            float left = 8.0f + direction * (0.6f + progress * 1.9f);
            float top = 3.0f + static_cast<float>((hash >> 5) % 10U)
                + progress * progress * 9.0f - progress * 4.0f;
            float size = fragment % 3 == 0 ? 2.0f : 1.0f;
            Color color = fragment % 2 == 0 ? Color{253, 164, 67, 255} : Color{174, 63, 45, 255};
            tilePixel(x, y, pixel, left, top, size, size, withOpacity(color, 1.0f - progress));
        }
    }

    Texture2D loadCharacterTexture(const std::string& assetRoot, Skin skin)
    {
        const std::string path = assetRoot + "characters/" + skinAssetName(skin) + ".png";
        if (!FileExists(path.c_str())) { return {}; }
        Texture2D texture = LoadTexture(path.c_str());
        if (texture.id == 0) { return {}; }
        if (texture.width != 48 || texture.height != 64)
        {
            UnloadTexture(texture);
            return {};
        }
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        return texture;
    }

    struct MedalArt
    {
        Color edge;
        Color face;
        Color shine;
    };

    MedalArt medalArt(Medal medal)
    {
        if (medal == Medal::Gold) { return {{157, 84, 29, 255}, {229, 154, 55, 255}, {255, 225, 137, 255}}; }
        if (medal == Medal::Silver) { return {{91, 103, 125, 255}, {160, 178, 189, 255}, {232, 239, 234, 255}}; }
        return {{103, 56, 47, 255}, {185, 101, 65, 255}, {248, 170, 110, 255}};
    }

    void drawMedal(float x, float y, float pixel, Medal medal)
    {
        MedalArt colors = medalArt(medal);
        tilePixel(x, y, pixel, 2, 0, 4, 6, {125, 57, 52, 255});
        tilePixel(x, y, pixel, 10, 0, 4, 6, {125, 57, 52, 255});
        tilePixel(x, y, pixel, 3, 0, 2, 5, {190, 79, 58, 255});
        tilePixel(x, y, pixel, 11, 0, 2, 5, {190, 79, 58, 255});
        tilePixel(x, y, pixel, 5, 3, 6, 13, colors.edge);
        tilePixel(x, y, pixel, 3, 5, 10, 10, colors.edge);
        tilePixel(x, y, pixel, 2, 7, 12, 6, colors.edge);
        tilePixel(x, y, pixel, 5, 4, 6, 11, colors.face);
        tilePixel(x, y, pixel, 4, 6, 8, 7, colors.face);
        tilePixel(x, y, pixel, 5, 4, 6, 1, colors.shine);
        tilePixel(x, y, pixel, 3, 7, 1, 4, colors.shine);
        tilePixel(x, y, pixel, 6, 13, 4, 1, colors.edge);
        if (medal == Medal::Gold)
        {
            tilePixel(x, y, pixel, 7, 6, 2, 6, colors.shine);
            tilePixel(x, y, pixel, 5, 8, 6, 2, colors.shine);
        }
        else if (medal == Medal::Silver)
        {
            tilePixel(x, y, pixel, 7, 6, 2, 4, colors.shine);
            tilePixel(x, y, pixel, 6, 8, 4, 3, colors.shine);
            tilePixel(x, y, pixel, 7, 11, 2, 1, colors.shine);
        }
        else
        {
            tilePixel(x, y, pixel, 7, 7, 2, 4, colors.shine);
            tilePixel(x, y, pixel, 6, 11, 4, 1, colors.shine);
        }
    }

    void drawAchievementIcon(float x, float y, float pixel)
    {
        tilePixel(x, y, pixel, 4, 1, 8, 14, {127, 69, 49, 255});
        tilePixel(x, y, pixel, 2, 3, 12, 10, {127, 69, 49, 255});
        tilePixel(x, y, pixel, 4, 3, 8, 10, {50, 32, 37, 255});
        tilePixel(x, y, pixel, 7, 4, 2, 4, {255, 133, 59, 255});
        tilePixel(x, y, pixel, 6, 7, 5, 4, {255, 133, 59, 255});
        tilePixel(x, y, pixel, 7, 10, 3, 2, {255, 133, 59, 255});
        tilePixel(x, y, pixel, 8, 8, 1, 3, {255, 229, 146, 255});
    }
}

GameViewLayout createGameViewLayout(int screenWidth, int screenHeight,
    const GameLogic& game, const GameVisualState& visualState)
{
    GameViewLayout layout = {};
    layout.scale = std::max(0.001f, std::min(static_cast<float>(screenWidth) / 1600.0f,
        static_cast<float>(screenHeight) / 900.0f));
    layout.viewport = {std::floor(std::max(0.0f, static_cast<float>(screenWidth) - 1600.0f * layout.scale) * 0.5f),
        std::floor(std::max(0.0f, static_cast<float>(screenHeight) - 900.0f * layout.scale) * 0.5f),
        1600.0f * layout.scale, 900.0f * layout.scale};
    layout.hud = scaledRectangle(layout, 32, 28, 1536, 112);
    layout.world = scaledRectangle(layout, 34, 182, 1170, 660);
    layout.map = scaledRectangle(layout, 1266, 220, 264, 264);
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

GameArt::GameArt(Skin skin) : titleFont(GetFontDefault()), bodyFont(GetFontDefault())
{
    const std::string assetRoot = std::string(GetApplicationDirectory()) + "assets/";
    playerTexture = loadCharacterTexture(assetRoot, skin);
    if (playerTexture.id == 0 && skin != Skin::Explorer)
    {
        playerTexture = loadCharacterTexture(assetRoot, Skin::Explorer);
    }

    int codepoints[95] = {};
    for (int codepointIndex = 0; codepointIndex < 95; codepointIndex++)
    {
        codepoints[codepointIndex] = codepointIndex + 32;
    }
    std::string titlePath = assetRoot + "fonts/PressStart2P-Regular.ttf";
    if (FileExists(titlePath.c_str()))
    {
        titleFont = LoadFontEx(titlePath.c_str(), 64, codepoints, 95);
        ownsTitleFont = titleFont.texture.id != 0 && titleFont.texture.id != GetFontDefault().texture.id;
        if (ownsTitleFont) { SetTextureFilter(titleFont.texture, TEXTURE_FILTER_POINT); }
        else { titleFont = GetFontDefault(); }
    }
    std::string bodyPath = assetRoot + "fonts/Rajdhani-Medium.ttf";
    if (FileExists(bodyPath.c_str()))
    {
        bodyFont = LoadFontEx(bodyPath.c_str(), 32, codepoints, 95);
        ownsBodyFont = bodyFont.texture.id != 0 && bodyFont.texture.id != GetFontDefault().texture.id;
        if (ownsBodyFont) { SetTextureFilter(bodyFont.texture, TEXTURE_FILTER_BILINEAR); }
        else { bodyFont = GetFontDefault(); }
    }
}

GameArt::~GameArt()
{
    if (playerTexture.id != 0) { UnloadTexture(playerTexture); }
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

void GameArt::drawHud(const GameLogic& game, const GameVisualState& state, const GameViewLayout& layout) const
{
    panel(layout, 32, 28, 1536, 112);
    Color cream = {241, 222, 187, 255};
    Color muted = {158, 145, 132, 255};
    Color amber = {255, 188, 87, 255};
    label(layout, "WOLCAN", 54, 49, 18, amber, true);
    label(layout, TextFormat("EXPEDITION  %02i", game.getLevelNumber()), 54, 86, 19, cream);
    const int dividerCount = 7;
    const float dividers[dividerCount] = {210, 382, 540, 658, 846, 1034, 1251};
    for (int dividerIndex = 0; dividerIndex < dividerCount; dividerIndex++)
    {
        DrawRectangleRec(scaledRectangle(layout, dividers[dividerIndex], 50, 1, 66), {80, 62, 52, 255});
    }
    label(layout, "DIFFICULTY", 230, 48, 18, muted);
    label(layout, difficultyName(game.getMaze().getDifficulty()), 230, 76, 28, cream);
    label(layout, "ROW / COLUMN", 402, 48, 18, muted);
    label(layout, TextFormat("%02i : %02i", game.getPlayer().getRow() + 1, game.getPlayer().getColumn() + 1),
        402, 76, 29, cream);
    label(layout, "MOVES", 560, 48, 18, muted);
    label(layout, TextFormat("%i", game.getMoveCount()), 560, 76, 29, cream);
    char elapsed[32] = {};
    char best[32] = {};
    formatTime(game.getElapsedSeconds(), elapsed, sizeof(elapsed));
    formatTime(game.getBestSeconds(), best, sizeof(best));
    label(layout, "ELAPSED", 678, 48, 18, muted);
    label(layout, elapsed, 678, 76, 29, amber);
    label(layout, "BEST  /  THIS SEED", 866, 48, 17, muted);
    label(layout, best, 866, 76, 29, cream);
    label(layout, TextFormat("SEED  %u", static_cast<unsigned int>(game.getMaze().getSeed())), 1054, 48, 19, muted);
    label(layout, TextFormat("SECRETS  %i / %i", game.getSecretsFound(), game.getMaze().getFalseWallCount()),
        1054, 84, 20, {114, 192, 170, 255});

    float health = std::clamp(state.displayedHealth, 0.0f, 100.0f) * 0.01f;
    float damage = std::clamp(state.damageFlash, 0.0f, 1.0f);
    Color barColor = {static_cast<unsigned char>(238.0f - std::max(0.0f, health - 0.55f) * 247.0f),
        static_cast<unsigned char>(70.0f + health * 110.0f),
        static_cast<unsigned char>(57.0f + std::max(0.0f, health - 0.55f) * 180.0f), 255};
    label(layout, game.isInLava() ? "HEALTH  /  IN LAVA" : "HEALTH", 1271, 45, 18, game.isInLava() ? amber : muted);
    label(layout, TextFormat("%i / 100 HP", static_cast<int>(std::ceil(game.getHealth()))),
        1271, 69, 23, cream);
    DrawRectangleRec(scaledRectangle(layout, 1271, 101, 159, 13), {61, 45, 44, 255});
    DrawRectangleRec(scaledRectangle(layout, 1272, 102, 157, 11), {13, 15, 21, 255});
    if (health > 0.0f)
    {
        DrawRectangleRec(scaledRectangle(layout, 1272, 102, 157 * health, 11), barColor);
        DrawRectangleRec(scaledRectangle(layout, 1272, 102, 157 * health, 3), {255, 243, 181, 77});
    }
    if (damage > 0.0f)
    {
        DrawRectangleRec(scaledRectangle(layout, 1271, 101, 159, 13), withOpacity({254, 74, 47, 153}, damage));
    }
    DrawRectangleRec(scaledRectangle(layout, 1447, 50, 1, 66), {80, 62, 52, 255});
    drawItem(layout.viewport.x + 1455.0f * layout.scale, layout.viewport.y + 47.0f * layout.scale,
        1.25f * layout.scale, ItemKind::Crystal, 0.9f);
    label(layout, "FINDS", 1476, 45, 18, muted);
    label(layout, TextFormat("%i / %i", game.getItemsCollected(), game.getItemCount()),
        1463, 75, 25, state.pickupFlash > 0.0f ? amber : cream);
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
    DrawRectangleRec(scaledRectangle(layout, 1250, 498, 8, 8), {249, 236, 196, 255});
    label(layout, "YOU", 1265, 490, 20, muted);
    DrawRectangleRec(scaledRectangle(layout, 1453, 498, 8, 8), {245, 184, 79, 255});
    label(layout, "EXIT", 1468, 490, 20, muted);
    DrawRectangleRec(scaledRectangle(layout, 1250, 520, 296, 1), {76, 60, 51, 255});

    MedalTargets targets = medalTargets(game.getShortestPath(), maze.getDifficulty());
    char goldTime[32] = {};
    char silverTime[32] = {};
    formatTime(targets.goldSeconds, goldTime, sizeof(goldTime));
    formatTime(targets.silverSeconds, silverTime, sizeof(silverTime));
    label(layout, "MEDALS", 1250, 529, 18, cream);
    label(layout, "MAX TIME", 1372, 529, 17, muted);
    label(layout, "MAX STEPS", 1465, 529, 17, muted);
    label(layout, "Gold", 1250, 553, 21, medalArt(Medal::Gold).shine);
    label(layout, goldTime, 1372, 553, 20, cream);
    label(layout, TextFormat("%i", targets.goldSteps), 1465, 553, 20, cream);
    label(layout, "Silver", 1250, 578, 21, medalArt(Medal::Silver).shine);
    label(layout, silverTime, 1372, 578, 20, cream);
    label(layout, TextFormat("%i", targets.silverSteps), 1465, 578, 20, cream);
    label(layout, "Bronze", 1250, 603, 21, medalArt(Medal::Bronze).shine);
    label(layout, "Finish the maze", 1372, 603, 20, cream);
    DrawRectangleRec(scaledRectangle(layout, 1250, 633, 296, 1), {76, 60, 51, 255});

    if (maze.getDifficulty() == Difficulty::ExtraHard)
    {
        label(layout, "Cracked floors can collapse.", 1250, 643, 19, muted);
        label(layout, "Orange flashes warn of spikes.", 1250, 665, 19, muted);
        label(layout, "Mineral seams hide paths.", 1250, 687, 19, muted);
    }
    else
    {
        label(layout, "FIND THE SUNLIT GATE", 1250, 643, 17, {255, 188, 87, 255});
        label(layout, "Mineral seams hide paths.", 1250, 665, 19, muted);
        label(layout, "Walk into cracked walls.", 1250, 687, 19, muted);
    }
    DrawRectangleRec(scaledRectangle(layout, 1250, 710, 296, 1), {76, 60, 51, 255});
    label(layout, "ARROWS", 1250, 716, 19, cream);
    label(layout, "Move", 1380, 716, 19, muted);
    label(layout, "P", 1250, 738, 19, cream);
    label(layout, "Pause / resume", 1380, 738, 19, muted);
    label(layout, "R  /  T", 1250, 760, 19, cream);
    label(layout, "New / same seed", 1380, 760, 19, muted);
    label(layout, "ESC", 1250, 782, 19, cream);
    label(layout, "Main menu", 1380, 782, 19, muted);
    label(layout, "N", 1250, 804, 19, cream);
    label(layout, "Next after escape", 1380, 804, 19, muted);
}

void GameArt::drawOverlay(const GameLogic& game, const GameVisualState& state,
    const GameViewLayout& layout, const Progression* progression) const
{
    if (!game.isPaused() && !game.hasWon()) { return; }
    DrawRectangleRec(layout.world, {8, 10, 17, 170});
    Color amber = {255, 194, 101, 255};
    Color cream = {248, 226, 183, 255};
    Color muted = {171, 154, 137, 255};
    Color teal = {120, 201, 173, 255};
    if (!game.hasWon())
    {
        panel(layout, 303, 385, 632, 254);
        DrawRectangleRec(scaledRectangle(layout, 553, 385, 132, 3), amber);
        centered(layout, "PAUSED", 619, 420, 30, amber, true);
        centered(layout, "P   RESUME EXPEDITION", 619, 508, 24, teal);
        centered(layout, "R  NEW SEED     T  REPLAY     ESC  MENU", 619, 594, 19, muted);
        return;
    }

    int achievementCount = progression == nullptr ? 0 : std::min(32, progression->getAchievementCount());
    int unlocked = 0;
    for (int achievementIndex = 0; achievementIndex < achievementCount; achievementIndex++)
    {
        if ((state.rewards.newAchievements & (std::uint32_t{1} << achievementIndex)) != 0U) { unlocked++; }
    }
    int unlockRows = (unlocked + 1) / 2;
    float height = 394.0f + (unlocked > 0 ? 40.0f + 23.0f * static_cast<float>(unlockRows) : 0.0f);
    float top = 512.0f - height * 0.5f;
    panel(layout, 219, top, 800, height);
    DrawRectangleRec(scaledRectangle(layout, 547, top, 144, 3), amber);
    centered(layout, "ISLAND ESCAPED", 619, top + 34, 25, amber, true);
    DrawRectangleRec(scaledRectangle(layout, 247, top + 94, 744, 115), {39, 28, 31, 255});
    DrawRectangleRec(scaledRectangle(layout, 247, top + 208, 744, 1), {98, 63, 48, 255});

    char elapsed[32] = {};
    char best[32] = {};
    formatTime(game.getElapsedSeconds(), elapsed, sizeof(elapsed));
    formatTime(game.getBestSeconds(), best, sizeof(best));
    label(layout, "FINAL TIME", 555, top + 118, 19, muted);
    label(layout, elapsed, 555, top + 144, 37, cream);
    label(layout, "STEPS", 786, top + 118, 19, muted);
    label(layout, TextFormat("%i", game.getMoveCount()), 786, top + 144, 37, cream);

    if (progression != nullptr)
    {
        MedalArt colors = medalArt(state.rewards.medal);
        float entrance = std::clamp(state.resultTime / 0.55f, 0.0f, 1.0f);
        float settle = 1.0f - entrance;
        float size = 86.0f - 28.0f * settle * settle + std::sin(entrance * 3.1415927f) * 11.0f;
        Rectangle badge = scaledRectangle(layout, 313.0f - size * 0.5f,
            top + 151.0f - size * 0.5f + 24.0f * settle * settle, size, size);
        drawMedal(badge.x, badge.y, badge.width / 16.0f, state.rewards.medal);
        label(layout, "MEDAL", 379, top + 118, 19, muted);
        label(layout, medalName(state.rewards.medal), 379, top + 144, 37, colors.shine);
    }

    label(layout, "PERSONAL BEST  /  THIS SEED", 254, top + 226, 18, teal);
    label(layout, TextFormat("Time  %s", best), 254, top + 251, 25, cream);
    if (progression != nullptr)
    {
        label(layout, TextFormat("Medal  %s", medalName(state.rewards.bestMedal)), 555, top + 251, 25, cream);
        label(layout, TextFormat("Steps  %i", state.rewards.bestSteps), 786, top + 251, 25, cream);
    }
    bool timeSaveFailed = !game.wasRecordSaved();
    bool profileSaveFailed = progression != nullptr && !progression->wasSaved();
    if (timeSaveFailed || profileSaveFailed)
    {
        const char* warning = "Progress could not be saved";
        if (timeSaveFailed)
        {
            warning = profileSaveFailed ? "Time and progress could not be saved" : "Time record could not be saved";
        }
        label(layout, warning, 254, top + 284, 16, {235, 158, 104, 255});
    }

    if (unlocked > 0)
    {
        label(layout, "NEW ACHIEVEMENTS", 254, top + 310, 18, amber);
        int listed = 0;
        for (int achievementIndex = 0; achievementIndex < achievementCount; achievementIndex++)
        {
            if ((state.rewards.newAchievements & (std::uint32_t{1} << achievementIndex)) == 0U) { continue; }
            float x = 254.0f + static_cast<float>(listed % 2) * 373.0f;
            float y = top + 335.0f + static_cast<float>(listed / 2) * 23.0f;
            Rectangle mark = scaledRectangle(layout, x + 1.0f, y + 6.0f, 6, 6);
            DrawRectangleRec(mark, amber);
            label(layout, progression->getAchievement(achievementIndex).name, x + 14.0f, y, 18, cream);
            listed++;
        }
    }

    DrawRectangleRec(scaledRectangle(layout, 247, top + height - 82, 744, 1), {98, 63, 48, 255});
    centered(layout, "N   NEXT EXPEDITION", 619, top + height - 68, 24, amber);
    centered(layout, "R  NEW SEED     T  REPLAY     ESC  MENU", 619, top + height - 43, 19,
        muted);
}

void GameArt::drawAchievementToast(const GameVisualState& state, const GameViewLayout& layout,
    const Progression& progression) const
{
    if (state.toastAchievement < 0 || state.toastAchievement >= progression.getAchievementCount()
        || state.toastTime >= 4.0f) { return; }
    const auto achievement = progression.getAchievement(state.toastAchievement);
    float transition = std::clamp(std::min(state.toastTime, 4.0f - state.toastTime) / 0.28f, 0.0f, 1.0f);
    float slide = 1.0f - transition;
    float top = 233.0f - 16.0f * slide * slide;
    panel(layout, 1245, top, 307, 153);
    DrawRectangleRec(scaledRectangle(layout, 1245, top, 307 * transition, 2), {235, 149, 63, 255});
    Rectangle icon = scaledRectangle(layout, 1254, top + 11, 43, 43);
    drawAchievementIcon(icon.x, icon.y, icon.width / 16.0f);
    label(layout, "ACHIEVEMENT UNLOCKED", 1300, top + 19, 17, {255, 188, 87, 255});

    const float textWidth = 276.0f * layout.scale;
    float nameWidth = MeasureTextEx(bodyFont, achievement.name, 22.0f * layout.scale, 0.5f * layout.scale).x;
    float nameSize = nameWidth > textWidth ? 22.0f * textWidth / nameWidth : 22.0f;
    label(layout, achievement.name, 1261, top + 55, nameSize, {246, 223, 181, 255});

    std::string remaining = achievement.description;
    for (int line = 0; line < 3; line++)
    {
        if (remaining.empty()) { break; }
        int length = static_cast<int>(remaining.size());
        int end = length;
        float fontSize = 19.0f * layout.scale;
        float spacing = 0.5f * layout.scale;
        if (MeasureTextEx(bodyFont, remaining.c_str(), fontSize, spacing).x > textWidth)
        {
            for (int characterIndex = 0; characterIndex < length; characterIndex++)
            {
                if (remaining[characterIndex] != ' ') { continue; }
                std::string nextLine = remaining.substr(0, characterIndex);
                if (MeasureTextEx(bodyFont, nextLine.c_str(), fontSize, spacing).x > textWidth) { break; }
                end = characterIndex;
            }
        }
        label(layout, remaining.substr(0, end).c_str(), 1261, top + 84.0f + static_cast<float>(line) * 19.0f,
            19, {186, 166, 144, 255});
        if (end == length) { break; }
        remaining.erase(0, end + 1);
    }
}

void GameArt::draw(const GameLogic& game, const GameVisualState& visualState, const Progression* progression) const
{
    GameViewLayout layout = createGameViewLayout(GetScreenWidth(), GetScreenHeight(), game, visualState);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {7, 8, 12, 255});
    DrawRectangleRec(layout.viewport, {17, 17, 22, 255});
    DrawRectangleRec(scaledRectangle(layout, 0, 0, 1600, 5), {112, 53, 34, 255});
    drawHud(game, visualState, layout);
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
    float pixel = static_cast<float>(layout.tileSize) / 16.0f;
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
                unsigned int hash = tileHash(row, column);
                CellFeature feature = game.getCellFeature(row, column);
                if (feature.hazard == HazardKind::Lava
                    || (feature.hazard == HazardKind::Fragile && feature.phase == HazardPhase::Active))
                {
                    drawLava(x, y, pixel, hash, visualState.idleTime);
                }
                else
                {
                    drawFloor(x, y, layout.tileSize, hash);
                }
                if (cell == 'X') { drawBoulder(x, y, layout.tileSize); }
                if (cell == '+')
                {
                    tilePixel(x, y, pixel, 0, 4, 2, 8, {58, 113, 109, 195});
                    tilePixel(x, y, pixel, 14, 4, 2, 8, {58, 113, 109, 195});
                }
                if (feature.hazard == HazardKind::Fragile && feature.phase != HazardPhase::Active)
                {
                    drawFragileFloor(x, y, pixel, feature);
                }
                if (feature.hazard == HazardKind::Spikes) { drawWallSpikes(x, y, pixel, feature); }
                if (feature.item != ItemKind::None)
                {
                    float pulse = std::max(0.0f, std::sin(visualState.idleTime * 2.45f + static_cast<float>(hash % 27U)));
                    float glow = 0.16f + 0.84f * pulse * pulse * pulse * pulse;
                    drawItem(x, y, pixel, feature.item, glow);
                    if (pulse > 0.95f)
                    {
                        tilePixel(x, y, pixel, 10, 4, 3, 1, withOpacity({255, 232, 174, 255}, glow));
                        tilePixel(x, y, pixel, 11, 3, 1, 3, withOpacity({255, 232, 174, 255}, glow));
                    }
                }
            }
        }
    }
    GridPosition start = maze.getStart();
    GridPosition exit = maze.getExit();
    drawMarker(layout.world.x + static_cast<float>(start.column * layout.tileSize) - layout.cameraX,
        layout.world.y + static_cast<float>(start.row * layout.tileSize) - layout.cameraY, layout.tileSize, false, visualState.idleTime);
    drawMarker(layout.world.x + static_cast<float>(exit.column * layout.tileSize) - layout.cameraX,
        layout.world.y + static_cast<float>(exit.row * layout.tileSize) - layout.cameraY, layout.tileSize, true, visualState.idleTime);
    float playerX = std::round(layout.world.x + visualState.playerColumn * static_cast<float>(layout.tileSize) - layout.cameraX);
    float playerY = std::round(layout.world.y + visualState.playerRow * static_cast<float>(layout.tileSize) - layout.cameraY);
    float deathProgress = game.isDying() ? game.getDeathProgress() : 0.0f;
    drawExplorer(playerX, playerY, layout.tileSize, visualState, playerTexture, deathProgress, game.isInLava());
    if (game.isDying()) { drawDeath(playerX, playerY, pixel, deathProgress); }
    if (visualState.pickupFlash > 0.0f)
    {
        GridPosition collected = game.getLastCollectedPosition();
        if (collected.row >= 0 && collected.column >= 0)
        {
            drawPickup(layout.world.x + static_cast<float>(collected.column * layout.tileSize) - layout.cameraX,
                layout.world.y + static_cast<float>(collected.row * layout.tileSize) - layout.cameraY,
                pixel, std::clamp(visualState.pickupFlash, 0.0f, 1.0f));
        }
    }
    EndScissorMode();
    drawMap(game, visualState, layout);
    label(layout, "VOLCANIC ISLE  /  FIELD NOTES", 34, 863, 16, {125, 116, 108, 255});
    label(layout, "F11  FULLSCREEN", 1408, 863, 16, {153, 136, 115, 255});
    drawOverlay(game, visualState, layout, progression);
    if (progression != nullptr) { drawAchievementToast(visualState, layout, *progression); }
}
