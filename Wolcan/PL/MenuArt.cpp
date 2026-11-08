#include "MenuArt.h"

#include <algorithm>
#include <cmath>
#include <string>

Color menuOpacity(Color color, float opacity)
{
    color.a = static_cast<unsigned char>(static_cast<float>(color.a) * std::clamp(opacity, 0.0f, 1.0f));
    return color;
}

Color menuBlend(Color firstColor, Color secondColor, float amount)
{
    float blend = std::clamp(amount, 0.0f, 1.0f);
    return {
        static_cast<unsigned char>(firstColor.r + (secondColor.r - firstColor.r) * blend),
        static_cast<unsigned char>(firstColor.g + (secondColor.g - firstColor.g) * blend),
        static_cast<unsigned char>(firstColor.b + (secondColor.b - firstColor.b) * blend),
        static_cast<unsigned char>(firstColor.a + (secondColor.a - firstColor.a) * blend)};
}

MenuArt::MenuArt() : titleFont(GetFontDefault()), bodyFont(GetFontDefault())
{
    const std::string assetRoot = std::string(GetApplicationDirectory()) + "assets/";
    std::string backgroundPath = assetRoot + "volcano-island.jpg";
    if (FileExists(backgroundPath.c_str()))
    {
        background = LoadTexture(backgroundPath.c_str());
        if (background.id != 0) { SetTextureFilter(background, TEXTURE_FILTER_POINT); }
    }

    // ASCII plus the en dash used by the discreet fullscreen instruction.
    int codepoints[96] = {};
    const int asciiCount = 95;
    for (int codepointIndex = 0; codepointIndex < asciiCount; codepointIndex++)
    {
        codepoints[codepointIndex] = codepointIndex + 32;
    }
    codepoints[95] = 0x2013;

    std::string titlePath = assetRoot + "fonts/PressStart2P-Regular.ttf";
    if (FileExists(titlePath.c_str()))
    {
        titleFont = LoadFontEx(titlePath.c_str(), 96, codepoints, 96);
        ownsTitleFont = titleFont.texture.id != 0 && titleFont.texture.id != GetFontDefault().texture.id;
        if (ownsTitleFont) { SetTextureFilter(titleFont.texture, TEXTURE_FILTER_POINT); }
        else { titleFont = GetFontDefault(); }
    }
    std::string bodyPath = assetRoot + "fonts/Rajdhani-Medium.ttf";
    if (FileExists(bodyPath.c_str()))
    {
        bodyFont = LoadFontEx(bodyPath.c_str(), 64, codepoints, 96);
        ownsBodyFont = bodyFont.texture.id != 0 && bodyFont.texture.id != GetFontDefault().texture.id;
        if (ownsBodyFont) { SetTextureFilter(bodyFont.texture, TEXTURE_FILTER_BILINEAR); }
        else { bodyFont = GetFontDefault(); }
    }
}

MenuArt::~MenuArt()
{
    if (background.id != 0) { UnloadTexture(background); }
    if (ownsTitleFont) { UnloadFont(titleFont); }
    if (ownsBodyFont) { UnloadFont(bodyFont); }
}

void MenuArt::drawCentered(Font font, const char* text, float centerX, float top,
    float fontSize, float spacing, Color color) const
{
    Vector2 textSize = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 position = {std::round(centerX - textSize.x * 0.5f), std::round(top)};
    DrawTextEx(font, text, position, fontSize, spacing, color);
}

void MenuArt::drawButton(Rectangle rectangle, int buttonIndex, const MenuVisualState& visualState,
    float scale, bool enabled, float opacity) const
{
    float hover = visualState.hoverAmounts[buttonIndex];
    float press = visualState.pressAmounts[buttonIndex];
    float growth = 3.0f * scale * hover;
    rectangle.x -= growth;
    rectangle.width += growth * 2.0f;
    rectangle.y += 2.0f * scale * press;

    Color amber = {255, 182, 73, 255};
    Color stone = {53, 28, 21, 255};
    Color border = menuBlend({110, 58, 32, 255}, amber, hover * 0.8f);
    if (!enabled) { opacity *= 0.35f; }

    if (hover > 0.01f && enabled)
    {
        DrawRectangleRec({rectangle.x - 3.0f * scale, rectangle.y - 3.0f * scale,
            rectangle.width + 6.0f * scale, rectangle.height + 6.0f * scale},
            menuOpacity({255, 120, 26, 35}, opacity * hover));
    }
    DrawRectangleRec({rectangle.x, rectangle.y + 5.0f * scale, rectangle.width, rectangle.height},
        menuOpacity({8, 4, 3, 210}, opacity));
    DrawRectangleRec(rectangle, menuOpacity(border, opacity));
    Rectangle inset = {rectangle.x + scale, rectangle.y + scale, rectangle.width - 2.0f * scale,
        rectangle.height - 2.0f * scale};
    bool isPrimary = buttonIndex == static_cast<int>(MenuAction::Start);
    Color fill = isPrimary ? menuBlend({231, 139, 44, 255}, {255, 192, 94, 255}, hover)
        : menuBlend(stone, {91, 44, 25, 255}, hover * 0.8f);
    fill = menuBlend(fill, {98, 44, 20, 255}, press * 0.22f);
    DrawRectangleRec(inset, menuOpacity(fill, opacity));
    DrawRectangleRec({inset.x, inset.y, inset.width, 2.0f * scale},
        menuOpacity(isPrimary ? Color{255, 227, 163, 220} : Color{143, 81, 45, 160}, opacity));

    Color labelColor = isPrimary ? Color{39, 18, 10, 255} : Color{236, 207, 171, 255};
    if (buttonIndex < 2)
    {
        // Three square steps make a small pixel-art chevron.
        const int stepCount = 3;
        for (int stepIndex = 0; stepIndex < stepCount; stepIndex++)
        {
            float direction = buttonIndex == 0 ? -1.0f : 1.0f;
            float offset = static_cast<float>(stepIndex);
            float pixelX = rectangle.x + rectangle.width * 0.5f + direction * (1.0f - offset) * 3.0f * scale;
            float pixelY = rectangle.y + rectangle.height * 0.5f;
            DrawRectangleRec({pixelX, pixelY - offset * 3.0f * scale - 2.0f * scale, 3.0f * scale, 3.0f * scale}, menuOpacity(amber, opacity));
            DrawRectangleRec({pixelX, pixelY + offset * 3.0f * scale - 2.0f * scale, 3.0f * scale, 3.0f * scale}, menuOpacity(amber, opacity));
        }
    }
    else
    {
        const char* label = isPrimary ? "ENTER THE MAZE" : "QUIT GAME";
        float fontSize = (isPrimary ? 29.0f : 23.0f) * scale;
        drawCentered(bodyFont, label, rectangle.x + rectangle.width * 0.5f,
            rectangle.y + (rectangle.height - fontSize) * 0.5f, fontSize, 2.0f * scale,
            menuOpacity(labelColor, opacity));
    }
}

void MenuArt::draw(const MenuLayout& layout, const MenuVisualState& visualState, int selectedSize, Difficulty difficulty) const
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float width = static_cast<float>(screenWidth);
    float height = static_cast<float>(screenHeight);
    float scale = layout.scale;
    if (background.id != 0)
    {
        DrawTexturePro(background, calculateCoverSource(background.width, background.height, screenWidth, screenHeight),
            {0, 0, width, height}, {0, 0}, 0.0f, {255, 255, 255, 255});
    }
    else
    {
        DrawRectangleGradientV(0, 0, screenWidth, screenHeight, {73, 20, 13, 255}, {18, 10, 8, 255});
    }
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, {12, 6, 5, 95}, {8, 5, 4, 190});

    const int sparkCount = 24;
    for (int sparkIndex = 0; sparkIndex < sparkCount; sparkIndex++)
    {
        float index = static_cast<float>(sparkIndex);
        float sparkX = std::fmod(index * 137.0f + 29.0f + std::sin(visualState.elapsed * 0.3f + index) * 10.0f + width, width);
        float sparkY = height - std::fmod(index * 47.0f + visualState.elapsed * (12.0f + static_cast<float>(sparkIndex % 7)), height);
        float sparkSize = (sparkIndex % 3 == 0 ? 2.0f : 1.0f) * scale;
        if (sparkX + sparkSize < width && sparkY + sparkSize < height)
        {
            DrawRectangleRec({sparkX, sparkY, sparkSize, sparkSize}, {255, 151, 54, 100});
        }
    }

    float appearance = std::clamp(visualState.elapsed / 0.32f, 0.0f, 1.0f);
    appearance = 1.0f - (1.0f - appearance) * (1.0f - appearance);
    Color cream = {244, 221, 184, 255};
    Color muted = {198, 157, 119, 255};
    Color gold = {255, 192, 95, 255};
    drawCentered(bodyFont, "A VOLCANIC MAZE ADVENTURE", layout.centerX, layout.top + 85.0f * scale,
        16.0f * scale, 3.0f * scale, menuOpacity(cream, appearance * 0.8f));

    float titleSize = std::floor(84.0f * scale / 8.0f) * 8.0f;
    float titleTop = layout.top + 124.0f * scale;
    float titleSpacing = 5.0f * scale;
    float glow = (0.78f + 0.12f * std::sin(visualState.elapsed * 1.7f)) * appearance;
    const int glowLayerCount = 4;
    for (int layerIndex = 0; layerIndex < glowLayerCount; layerIndex++)
    {
        float directionX = layerIndex % 2 == 0 ? -1.0f : 1.0f;
        float directionY = layerIndex < 2 ? -1.0f : 1.0f;
        drawCentered(titleFont, "WOLCAN", layout.centerX + directionX * 3.0f * scale,
            titleTop + directionY * 3.0f * scale, titleSize, titleSpacing, menuOpacity({255, 82, 15, 65}, glow));
    }
    drawCentered(titleFont, "WOLCAN", layout.centerX + 5.0f * scale, titleTop + 8.0f * scale,
        titleSize, titleSpacing, menuOpacity({67, 21, 10, 255}, appearance));
    drawCentered(titleFont, "WOLCAN", layout.centerX, titleTop, titleSize, titleSpacing, menuOpacity(gold, appearance));
    drawCentered(bodyFont, "Find your way. Escape the island.", layout.centerX, layout.top + 228.0f * scale,
        23.0f * scale, 0.8f * scale, menuOpacity(cream, appearance * 0.9f));

    Rectangle panel = layout.panel;
    DrawRectangleRec({panel.x + 6.0f * scale, panel.y + 8.0f * scale, panel.width, panel.height}, menuOpacity({5, 3, 2, 150}, appearance));
    DrawRectangleRec(panel, menuOpacity({112, 58, 31, 210}, appearance));
    DrawRectangleRec({panel.x + scale, panel.y + scale, panel.width - 2.0f * scale, panel.height - 2.0f * scale},
        menuOpacity({23, 14, 11, 235}, appearance));
    DrawRectangleRec({layout.centerX - 38.0f * scale, panel.y, 76.0f * scale, 2.0f * scale}, menuOpacity(gold, appearance));
    drawCentered(bodyFont, "MAZE SIZE", layout.centerX, layout.top + 307.0f * scale,
        16.0f * scale, 2.5f * scale, menuOpacity(muted, appearance));
    drawCentered(bodyFont, TextFormat("%02i  x  %02i", selectedSize, selectedSize), layout.centerX,
        layout.top + 341.0f * scale, 43.0f * scale, 2.0f * scale, menuOpacity(cream, appearance));
    DrawRectangleRec({panel.x + 38.0f * scale, layout.top + 438.0f * scale, panel.width - 76.0f * scale, scale},
        menuOpacity({107, 60, 35, 150}, appearance));
    const int buttonCount = 4;
    for (int buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
    {
        bool enabled = (buttonIndex != 0 || selectedSize > 10) && (buttonIndex != 1 || selectedSize < 50);
        drawButton(layout.buttons[buttonIndex], buttonIndex, visualState, scale, enabled, appearance);
    }
    const int modeCount = 3;
    const char* modeNames[modeCount] = {"1  EASY", "2  NORMAL", "3  HARD"};
    for (int modeIndex = 0; modeIndex < modeCount; modeIndex++)
    {
        int buttonIndex = modeIndex + static_cast<int>(MenuAction::Easy);
        Rectangle button = layout.buttons[buttonIndex];
        button.y += visualState.pressAmounts[buttonIndex] * 2.0f * scale;
        bool selected = static_cast<int>(difficulty) == modeIndex;
        Color fill = selected ? Color{205, 125, 43, 255}
            : menuBlend({49, 27, 19, 255}, {94, 50, 26, 255}, visualState.hoverAmounts[buttonIndex]);
        DrawRectangleRec(button, menuOpacity({118, 66, 35, 255}, appearance));
        DrawRectangleRec({button.x + scale, button.y + scale, button.width - 2.0f * scale, button.height - 2.0f * scale},
            menuOpacity(fill, appearance));
        drawCentered(bodyFont, modeNames[modeIndex], button.x + button.width * 0.5f,
            button.y + 6.0f * scale, 18.0f * scale, 0.3f * scale,
            menuOpacity(selected ? Color{33, 18, 11, 255} : cream, appearance));
    }
    drawCentered(bodyFont, "ARROWS: SIZE     ENTER: PLAY     ESC: QUIT", layout.centerX,
        layout.top + 580.0f * scale, 16.0f * scale, 0.7f * scale, menuOpacity(muted, appearance * 0.9f));

    float footerSize = std::clamp(18.0f * scale, 18.0f, 24.0f);
    float footerTop = height - footerSize - 14.0f;
    DrawTextEx(bodyFont, "F11 – Fullscreen", {24.0f, footerTop}, footerSize, 0.5f, {235, 211, 181, 170});
    const char* islandLabel = "VOLCANIC ISLE / 01";
    Vector2 labelSize = MeasureTextEx(bodyFont, islandLabel, footerSize - 1.0f, 1.0f);
    DrawTextEx(bodyFont, islandLabel, {width - labelSize.x - 24.0f, footerTop}, footerSize - 1.0f, 1.0f, {235, 211, 181, 140});
}
