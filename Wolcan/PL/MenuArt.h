#pragma once

#include "MenuLayout.h"
#include "GameTypes.h"

struct MenuVisualState
{
    float elapsed = 0.0f;
    float hoverAmounts[7] = {};
    float pressAmounts[7] = {};
};

class MenuArt
{
public:
    MenuArt();
    ~MenuArt();
    MenuArt(const MenuArt& other) = delete;
    MenuArt& operator=(const MenuArt& other) = delete;

    void draw(const MenuLayout& layout, const MenuVisualState& visualState, int selectedSize,
        Difficulty difficulty = Difficulty::Normal) const;

private:
    Texture2D background = {};
    Font titleFont = {};
    Font bodyFont = {};
    bool ownsTitleFont = false;
    bool ownsBodyFont = false;

    void drawCentered(Font font, const char* text, float centerX, float top,
        float fontSize, float spacing, Color color) const;
    void drawButton(Rectangle rectangle, int buttonIndex, const MenuVisualState& visualState,
        float scale, bool enabled, float opacity) const;
};
