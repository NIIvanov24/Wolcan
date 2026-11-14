#include "MenuArt.h"
#include "GameSettings.h"
#include "GameArt.h"

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
    if (previewTexture.id != 0) { UnloadTexture(previewTexture); }
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

void MenuArt::drawBackdrop(const MenuLayout& layout, const MenuVisualState& visualState) const
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

}

void MenuArt::draw(const MenuLayout& layout, const MenuVisualState& visualState, int selectedSize,
    Difficulty difficulty, bool hasProfile, const GameSettings* settings) const
{
    drawBackdrop(layout, visualState);
    float width = static_cast<float>(GetScreenWidth());
    float height = static_cast<float>(GetScreenHeight());
    float scale = layout.scale;
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
    const int modeCount = 4;
    const char* modeNames[modeCount] = {"1 EASY", "2 NORMAL", "3 HARD", "4 EXTRA HARD"};
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
            button.y + 7.0f * scale, 16.0f * scale, 0.2f * scale,
            menuOpacity(selected ? Color{33, 18, 11, 255} : cream, appearance));
    }
    if (hasProfile)
    {
        int achievements = static_cast<int>(MenuAction::Achievements);
        int character = static_cast<int>(MenuAction::Character);
        drawProfileButton(layout.buttons[achievements], "A  ACHIEVEMENTS", scale,
            visualState.hoverAmounts[achievements] > 0.25f, true);
        drawProfileButton(layout.buttons[character], "C  CHARACTER CUSTOMIZATION", scale,
            visualState.hoverAmounts[character] > 0.25f, true);
    }
    if (settings != nullptr)
    {
        int settingsButton = static_cast<int>(MenuAction::Settings);
        int help = static_cast<int>(MenuAction::Help);
        drawProfileButton(layout.buttons[settingsButton], "SETTINGS", scale, visualState.hoverAmounts[settingsButton] > 0.25f, true);
        drawProfileButton(layout.buttons[help], "?", scale, visualState.hoverAmounts[help] > 0.25f, true);
    }
    drawCentered(bodyFont, "ARROWS: SIZE     ENTER: PLAY     ESC: QUIT", layout.centerX,
        layout.top + (hasProfile || settings != nullptr ? 631.0f : 580.0f) * scale, 16.0f * scale, 0.7f * scale,
        menuOpacity(muted, appearance * 0.9f));

    float footerSize = std::clamp(18.0f * scale, 18.0f, 24.0f);
    float footerTop = height - footerSize - 14.0f;
    const char* fullscreenKey = settings == nullptr ? "F11" : keyName(settings->key(GameAction::Fullscreen));
    std::string fullscreenHint = std::string(fullscreenKey) + " – Fullscreen";
    DrawTextEx(bodyFont, fullscreenHint.c_str(), {24.0f, footerTop}, footerSize, 0.5f, {235, 211, 181, 170});
    const char* islandLabel = "VOLCANIC ISLE / 01";
    Vector2 labelSize = MeasureTextEx(bodyFont, islandLabel, footerSize - 1.0f, 1.0f);
    DrawTextEx(bodyFont, islandLabel, {width - labelSize.x - 24.0f, footerTop}, footerSize - 1.0f, 1.0f, {235, 211, 181, 140});
}

void MenuArt::drawProfileButton(Rectangle button, const char* label, float scale, bool hovered, bool enabled) const
{
    Color edge = hovered && enabled ? Color{228, 147, 68, 255} : Color{116, 69, 48, 255};
    Color fill = hovered && enabled ? Color{102, 53, 32, 255} : Color{52, 29, 21, 255};
    Color text = enabled ? Color{245, 218, 183, 255} : Color{150, 123, 108, 255};
    DrawRectangleRec(button, edge);
    DrawRectangleRec({button.x + scale, button.y + scale, button.width - 2.0f * scale,
        button.height - 2.0f * scale}, fill);
    float fontSize = 18.0f * scale;
    float textWidth = MeasureTextEx(bodyFont, label, fontSize, 0.2f * scale).x;
    if (textWidth > button.width - 10.0f * scale) { fontSize *= (button.width - 10.0f * scale) / textWidth; }
    drawCentered(bodyFont, label, button.x + button.width * 0.5f,
        button.y + (button.height - fontSize) * 0.5f, fontSize, 0.2f * scale, text);
}

void MenuArt::drawProfile(const MenuLayout& layout, const ProfileLayout& profileLayout,
    const MenuVisualState& visualState, const Progression& progression, MenuAction page, Skin preview,
    float previewTime, int hoveredButton)
{
    drawBackdrop(layout, visualState);
    Rectangle panel = profileLayout.panel;
    float scale = layout.scale;
    Color gold = {255, 190, 103, 255};
    Color cream = {245, 224, 197, 255};
    Color muted = {180, 153, 135, 255};
    auto rectangle = [&](float x, float y, float width, float height, Color color)
    {
        DrawRectangleRec({panel.x + x * scale, panel.y + y * scale, width * scale, height * scale}, color);
    };
    auto label = [&](const char* text, float x, float y, float size, Color color, float limit)
    {
        float fontSize = size * scale;
        float width = MeasureTextEx(bodyFont, text, fontSize, 0).x;
        if (width > limit * scale) { fontSize *= limit * scale / width; }
        DrawTextEx(bodyFont, text, {panel.x + x * scale, panel.y + y * scale}, fontSize, 0, color);
    };
    rectangle(5, 7, 860, 590, {7, 4, 5, 180});
    rectangle(0, 0, 860, 590, {129, 75, 48, 255});
    rectangle(1, 1, 858, 588, {29, 19, 18, 248});
    rectangle(28, 87, 804, 1, {124, 73, 49, 255});
    bool customization = page == MenuAction::Character;
    drawCentered(titleFont, customization ? "CHARACTER CUSTOMIZATION" : "ACHIEVEMENTS", layout.centerX,
        panel.y + 26.0f * scale, 24.0f * scale, 1.0f * scale, gold);

    if (!customization)
    {
        int count = progression.getAchievementCount();
        int unlockedCount = 0;
        for (int index = 0; index < count; index++)
        {
            AchievementInfo achievement = progression.getAchievement(index);
            if (achievement.unlocked) { unlockedCount++; }
            float x = 28.0f + static_cast<float>(index % 2) * 406.0f;
            float y = 98.0f + static_cast<float>(index / 2) * 60.0f;
            Color accent = achievement.unlocked ? gold : Color{134, 106, 96, 255};
            rectangle(x, y, 398, 54, {51, 34, 30, 255});
            rectangle(x, y, 2, 54, accent);
            Rectangle icon = {panel.x + (x + 8) * scale, panel.y + (y + 10) * scale, 33 * scale, 33 * scale};
            if (!drawSecretAchievementIcon(icon, achievement.name))
            {
                rectangle(x + 14, y + 13, 19, 13, accent);
                rectangle(x + 18, y + 26, 11, 7, accent);
                rectangle(x + 21, y + 33, 5, 6, accent);
                rectangle(x + 17, y + 39, 13, 3, accent);
            }
            label(achievement.name, x + 46, y + 7, 22, cream, 283);
            label(achievement.description, x + 46, y + 34, 17, muted, 340);
            const char* progressText = achievement.unlocked ? "DONE" : TextFormat("%i / %i", achievement.progress, achievement.target);
            label(progressText, x + 332, y + 10, 17, accent, 59);
        }
        int collectibles = progression.getCollectibleCount();
        drawCentered(bodyFont, TextFormat("%i / %i UNLOCKED   |   %i %s", unlockedCount, count, collectibles,
            collectibles == 1 ? "ARTIFACT" : "ARTIFACTS"), layout.centerX, panel.y + 57.0f * scale,
            21.0f * scale, 0.8f * scale, muted);
    }
    else
    {
        int skinCount = static_cast<int>(Skin::CinderWarden) + 1;
        for (int index = 0; index < skinCount; index++)
        {
            Skin skin = static_cast<Skin>(index);
            SkinInfo information = skinInfo(skin);
            float y = 101.0f + static_cast<float>(index) * 50.0f;
            bool previewed = preview == skin;
            rectangle(28, y, 420, 45, previewed ? Color{91, 49, 34, 255} : Color{50, 33, 29, 255});
            if (previewed) { rectangle(28, y, 3, 45, gold); }
            label(information.name, 43, y + 2, 22, cream, 273);
            label(information.requirement, 43, y + 23, 17, muted, 389);
            const char* status = skin == progression.getSelectedSkin() ? "EQUIPPED" :
                progression.isSkinUnlocked(skin) ? "AVAILABLE" : "LOCKED";
            label(status, 345, y + 8, 17, previewed ? gold : muted, 91);
        }
        bool unlocked = progression.isSkinUnlocked(preview);
        SkinInfo information = skinInfo(preview);
        rectangle(473, 106, 358, 244, {43, 27, 28, 255});
        rectangle(489, 122, 326, 212, {34, 23, 25, 255});
        rectangle(564, 275, 176, 5, {95, 52, 38, 255});
        rectangle(580, 280, 144, 8, {61, 35, 29, 255});
        if (!previewLoaded || loadedPreview != preview)
        {
            if (previewTexture.id != 0) { UnloadTexture(previewTexture); }
            previewTexture = {};
            loadedPreview = preview;
            previewLoaded = true;
            std::string path = std::string(GetApplicationDirectory()) + "assets/characters/" + skinAssetName(preview) + ".png";
            if (FileExists(path.c_str()))
            {
                previewTexture = LoadTexture(path.c_str());
                if (previewTexture.id != 0) { SetTextureFilter(previewTexture, TEXTURE_FILTER_POINT); }
            }
        }
        if (previewTexture.id != 0 && previewTexture.width == 48 && previewTexture.height == 64)
        {
            float fraction = std::clamp(previewTime / 0.23f, 0.0f, 1.0f);
            float size = 16.0f * std::max(1.0f, std::floor(8.0f * scale));
            int frame = static_cast<int>(previewTime * 5.0f) % 4;
            if (frame == 3) { frame = 0; }
            float direction = std::fmod(std::floor(previewTime / 1.5f) + 1.0f, 4.0f);
            DrawTexturePro(previewTexture, {16.0f * static_cast<float>(frame), 16.0f * direction, 16, 16},
                {std::round(panel.x + (652.0f + (1.0f - fraction) * 18.0f) * scale - size * 0.5f),
                    std::round(panel.y + 281.0f * scale - size), size, size},
                {0, 0}, 0, menuOpacity({255, 255, 255, 255}, fraction));
        }
        drawCentered(bodyFont, information.name, panel.x + 652.0f * scale, panel.y + 302.0f * scale,
            27.0f * scale, 0.4f * scale, gold);
        const char* labels[3] = {"<", ">", unlocked ? "EQUIP SKIN" : "LOCKED"};
        for (int index = 0; index < 3; index++)
        {
            int buttonIndex = index + static_cast<int>(ProfileAction::Previous);
            drawProfileButton(profileLayout.buttons[buttonIndex], labels[index], scale,
                hoveredButton == buttonIndex, index != 2 || unlocked);
        }
        label("LEFT / RIGHT  Browse     ENTER  Equip", 490, 421, 19, muted, 332);
    }

    int backButton = static_cast<int>(ProfileAction::Back);
    drawProfileButton(profileLayout.buttons[backButton], "ESC  BACK", scale, hoveredButton == backButton, true);
    if (!progression.wasSaved()) { label("PROFILE COULD NOT BE SAVED", 174, 543, 18, gold, 470); }
}

Rectangle MenuArt::optionRectangle(const MenuLayout& layout, float x, float y, float width, float height) const
{
    Rectangle panel = createProfileLayout(layout).panel;
    return {panel.x + x * layout.scale, panel.y + y * layout.scale, width * layout.scale, height * layout.scale};
}

void MenuArt::drawOptions(const MenuLayout& layout, const MenuVisualState& visualState, const GameSettings& settings,
    bool help, int editing, const char* message, Vector2 mouse) const
{
    drawBackdrop(layout, visualState);
    Rectangle panel = createProfileLayout(layout).panel;
    float scale = layout.scale;
    Color gold = {255, 190, 103, 255};
    Color cream = {245, 224, 197, 255};
    Color muted = {185, 160, 142, 255};
    auto block = [&](float x, float y, float width, float height, Color color)
    {
        DrawRectangleRec(optionRectangle(layout, x, y, width, height), color);
    };
    auto label = [&](const char* text, float x, float y, float size, Color color)
    {
        DrawTextEx(bodyFont, text, {panel.x + x * scale, panel.y + y * scale}, size * scale, 0, color);
    };
    auto button = [&](const char* text, float x, float y, float width, float height)
    {
        Rectangle bounds = optionRectangle(layout, x, y, width, height);
        drawProfileButton(bounds, text, scale, CheckCollisionPointRec(mouse, bounds), true);
    };
    DrawRectangleRec(panel, {129, 75, 48, 255});
    block(1, 1, 858, 588, {29, 19, 18, 247});
    drawCentered(titleFont, help ? "HELP & TIPS" : "SETTINGS", layout.centerX, panel.y + 29 * scale,
        25 * scale, scale, gold);
    block(28, 76, 804, 1, {124, 73, 49, 255});

    if (help)
    {
        const char* titles[] = {"THE MAZE", "DIFFICULTIES", "HEALTH", "LAVA AND SPIKES",
            "EXPLORATION", "MEDALS", "REWARDS", "CONTROLS"};
        const char* lines[8][2] = {
            {"Find the golden exit. Solid stone blocks movement.", "The minimap shows your position and the exit."},
            {"Easy has more loops. Normal adds boulders.", "Hard has longer routes. Extra Hard adds traps."},
            {"You start at 100 HP. Recover 4 HP every 2 seconds", "outside danger. At 0 HP, retry from the same entrance."},
            {"Escape lava quickly. It slows you and deals 7 HP/s.", "A wall flash warns of spikes: back away or wait."},
            {"Walk onto glinting artifacts. Try mineral seams.", "Boulders block passages. Find another route."},
            {"Gold and Silver require both limits beside the map.", "Bronze recognizes a completed maze."},
            {"See unlock conditions in Achievements.", "Equip unlocked skins in Character Customization."},
            {"", ""}
        };
        const unsigned short icons[8][7] = {
            {127, 65, 93, 85, 117, 65, 127}, {1, 3, 7, 15, 31, 63, 127},
            {54, 127, 127, 62, 28, 8, 0}, {8, 10, 28, 62, 62, 28, 8},
            {8, 28, 62, 127, 62, 28, 8}, {127, 99, 62, 28, 8, 8, 62},
            {127, 73, 127, 62, 42, 62, 0}, {8, 28, 62, 8, 8, 8, 0}
        };
        const int sectionCount = 8;
        for (int index = 0; index < sectionCount; index++)
        {
            float x = 28 + static_cast<float>(index % 2) * 406;
            float y = 99 + static_cast<float>(index / 2) * 103;
            block(x, y, 398, 95, {52, 33, 28, 255});
            const int iconSize = 7;
            for (int row = 0; row < iconSize; row++)
            {
                for (int column = 0; column < iconSize; column++)
                {
                    if ((icons[index][row] & (1u << column)) != 0u)
                    {
                        block(x + 9 + static_cast<float>(column) * 2, y + 12 + static_cast<float>(row) * 2,
                            2, 2, gold);
                    }
                }
            }
            label(titles[index], x + 30, y + 9, 21, gold);
            if (index == 7)
            {
                std::string controls = std::string("Move: ") + keyName(settings.key(GameAction::Up));
                controls += std::string(" / ") + keyName(settings.key(GameAction::Down));
                controls += std::string(" / ") + keyName(settings.key(GameAction::Left));
                controls += std::string(" / ") + keyName(settings.key(GameAction::Right));
                float width = MeasureTextEx(bodyFont, controls.c_str(), 17 * scale, 0).x;
                float fontSize = width > 374 * scale ? 17 * 374 * scale / width : 17;
                label(controls.c_str(), x + 12, y + 41, fontSize, cream);
                std::string extras = std::string("Pause: ") + keyName(settings.key(GameAction::Pause));
                extras += std::string("    Menu: ") + keyName(settings.key(GameAction::Menu));
                label(extras.c_str(), x + 12, y + 62, 17, muted);
            }
            else
            {
                label(lines[index][0], x + 12, y + 41, 17, cream);
                label(lines[index][1], x + 12, y + 62, 17, muted);
            }
        }
    }
    else
    {
        label("KEYBOARD BINDINGS", 30, 91, 23, gold);
        label("AUDIO", 486, 91, 23, gold);
        const int count = static_cast<int>(GameAction::Count);
        for (int index = 0; index < count; index++)
        {
            float y = 127 + static_cast<float>(index) * 33;
            GameAction action = static_cast<GameAction>(index);
            block(28, y, 420, 29, index == editing ? Color{102, 55, 33, 255} : Color{49, 32, 28, 255});
            std::string actionLabel = action == GameAction::Hammer ? "Use equipped tool" : actionName(action);
            label(actionLabel.c_str(), 40, y + 3, 19, cream);
            button(index == editing ? "PRESS A KEY" : keyName(settings.key(action)), 328, y, 120, 29);
        }
        const char* labels[2] = {"Music volume", "Sound effects volume"};
        const int volumes[2] = {settings.musicVolume, settings.effectsVolume};
        const bool mutedBuses[2] = {settings.musicMuted, settings.effectsMuted};
        for (int index = 0; index < 2; index++)
        {
            float y = 152 + static_cast<float>(index) * 139;
            block(474, y - 14, 358, 118, {51, 33, 29, 255});
            label(labels[index], 489, y, 22, cream);
            label(TextFormat("%i%%", volumes[index]), 756, y, 21, gold);
            block(489, y + 36, 308, 8, {81, 51, 43, 255});
            block(489, y + 36, 308 * static_cast<float>(volumes[index]) / 100, 8, gold);
            block(484 + 308 * static_cast<float>(volumes[index]) / 100, y + 30, 10, 20, cream);
            button(mutedBuses[index] ? "UNMUTE" : "MUTE", 489, y + 59, 116, 32);
        }
        button("RESET TO DEFAULT", 482, 459, 206, 33);
        label(message, 28, 502, 17, gold);
    }
    button("ESC  BACK", 28, 531, 124, 38);
    if (!settings.wasSaved()) { label("SETTINGS COULD NOT BE SAVED", 170, 542, 17, gold); }
}
