#pragma once

#include "raylib.h"

enum class MenuAction { None = -1, DecreaseSize, IncreaseSize, Start, Quit, Easy, Normal, Hard };

struct MenuLayout
{
    float scale;
    float top;
    float centerX;
    Rectangle panel;
    Rectangle buttons[7];
};

MenuLayout createMenuLayout(int screenWidth, int screenHeight);
Rectangle calculateCoverSource(int imageWidth, int imageHeight, int screenWidth, int screenHeight);
