#pragma once

#include "raylib.h"

enum class MenuAction { None = -1, DecreaseSize, IncreaseSize, Start, Quit, Easy, Normal, Hard, ExtraHard,
    Achievements, Character, Settings, Help, MiniGames, Count };

enum class ProfileAction { None = -1, Back, Previous, Next, Confirm, Count };
enum class MiniMenuAction { Lava, Spikes, Back, Count };

struct MenuLayout
{
    float scale;
    float top;
    float centerX;
    Rectangle panel;
    Rectangle buttons[static_cast<int>(MenuAction::Count)];
};

struct ProfileLayout
{
    Rectangle panel;
    Rectangle buttons[static_cast<int>(ProfileAction::Count)];
};

MenuLayout createMenuLayout(int screenWidth, int screenHeight);
ProfileLayout createProfileLayout(const MenuLayout& layout);
Rectangle achievementPageButton(const ProfileLayout& layout, float scale, bool next);
Rectangle miniMenuButton(const MenuLayout& layout, MiniMenuAction action);
Rectangle calculateCoverSource(int imageWidth, int imageHeight, int screenWidth, int screenHeight);
