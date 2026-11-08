#pragma once

#include "raylib.h"

enum class MenuAction { None = -1, DecreaseSize, IncreaseSize, Start, Quit };

struct MenuLayout
{
    float scale;
    float top;
    float centerX;
    Rectangle panel;
    Rectangle buttons[4];
};

MenuLayout createMenuLayout(int screenWidth, int screenHeight);
Rectangle calculateCoverSource(int imageWidth, int imageHeight, int screenWidth, int screenHeight);
