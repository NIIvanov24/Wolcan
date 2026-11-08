#pragma once

#include "MenuLayout.h"

struct MenuVisualState
{
    float elapsed = 0.0f;
    float hoverAmounts[4] = {};
    float pressAmounts[4] = {};
};

class MenuArt
{
public:
    MenuArt();
    ~MenuArt();
    MenuArt(const MenuArt& other) = delete;
    MenuArt& operator=(const MenuArt& other) = delete;

    void draw(const MenuLayout& layout, const MenuVisualState& visualState, int selectedSize) const;

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
