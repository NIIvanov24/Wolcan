#pragma once

#include "MenuLayout.h"
#include "Progression.h"

struct MenuVisualState
{
    float elapsed = 0.0f;
    float hoverAmounts[static_cast<int>(MenuAction::Count)] = {};
    float pressAmounts[static_cast<int>(MenuAction::Count)] = {};
};

class MenuArt
{
public:
    MenuArt();
    ~MenuArt();
    MenuArt(const MenuArt& other) = delete;
    MenuArt& operator=(const MenuArt& other) = delete;

    void draw(const MenuLayout& layout, const MenuVisualState& visualState, int selectedSize,
        Difficulty difficulty = Difficulty::Normal, bool hasProfile = false) const;
    void drawProfile(const MenuLayout& layout, const ProfileLayout& profileLayout, const MenuVisualState& visualState,
        const Progression& progression, MenuAction page, Skin preview, float previewTime, int hoveredButton);

private:
    Texture2D background = {};
    Texture2D previewTexture = {};
    Skin loadedPreview = Skin::Explorer;
    bool previewLoaded = false;
    Font titleFont = {};
    Font bodyFont = {};
    bool ownsTitleFont = false;
    bool ownsBodyFont = false;

    void drawCentered(Font font, const char* text, float centerX, float top,
        float fontSize, float spacing, Color color) const;
    void drawButton(Rectangle rectangle, int buttonIndex, const MenuVisualState& visualState,
        float scale, bool enabled, float opacity) const;
    void drawBackdrop(const MenuLayout& layout, const MenuVisualState& visualState) const;
    void drawProfileButton(Rectangle button, const char* label, float scale, bool hovered, bool enabled) const;
};
