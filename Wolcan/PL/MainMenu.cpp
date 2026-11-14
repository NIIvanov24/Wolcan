#include "MainMenu.h"
#include "GameAudio.h"
#include "GameSettings.h"
#include "MenuArt.h"
#include "WindowControls.h"

#include <algorithm>
#include <cmath>

namespace
{
void updateAudio(GameAudio* audio, const GameSettings* settings)
{
    if (audio != nullptr && settings != nullptr)
    {
        audio->update(settings->musicVolume, settings->effectsVolume, settings->musicMuted, settings->effectsMuted);
    }
}

void menuClick(GameAudio* audio)
{
    if (audio != nullptr) { audio->play(SoundCue::Menu); }
}

void runOptionsMenu(MenuArt& menuArt, GameSettings& settings, bool help, MenuVisualState& visualState, GameAudio* audio)
{
    int editing = -1;
    int slider = -1;
    bool volumeChanged = false;
    const char* message = "";
    while (!WindowShouldClose())
    {
        int inputKey = GetKeyPressed();
        visualState.elapsed += std::clamp(GetFrameTime(), 0.0f, 0.05f);
        MenuLayout layout = createMenuLayout(GetScreenWidth(), GetScreenHeight());
        Vector2 mouse = GetMousePosition();
        bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool held = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        // In the browser this also disables the trusted-event fullscreen handler.
        handleWindowShortcuts(&settings, editing >= 0);
        auto hit = [&](float x, float y, float width, float height)
        {
            return CheckCollisionPointRec(mouse, menuArt.optionRectangle(layout, x, y, width, height));
        };
        bool close = (editing < 0 && IsKeyPressed(KEY_ESCAPE)) || (pressed && hit(28, 531, 124, 38));
        bool clicked = close;
        if (!close && !help)
        {
            if (editing >= 0 && inputKey != KEY_NULL)
            {
                GameAction action = static_cast<GameAction>(editing);
                if (inputKey == KEY_ESCAPE && action != GameAction::Menu) { editing = -1; message = ""; }
                else if (settings.bind(action, inputKey))
                {
                    settings.save();
                    editing = -1;
                    clicked = true;
                    message = "";
                }
                else
                {
                    message = action == GameAction::Fullscreen ? "Choose an unused function key (F1 to F12)."
                        : "Choose an unused supported key, or press Esc to cancel.";
                }
            }
            const int count = static_cast<int>(GameAction::Count);
            for (int index = 0; index < count; index++)
            {
                if (pressed && hit(28, 127 + static_cast<float>(index) * 33, 420, 29))
                {
                    editing = index;
                    message = "Press the new key. Esc cancels or restores Return to menu.";
                    clicked = true;
                }
            }
            if (pressed && hit(482, 459, 206, 33))
            {
                settings.resetBindings();
                settings.save();
                editing = -1;
                message = "Default bindings restored.";
                clicked = true;
            }
            for (int index = 0; index < 2; index++)
            {
                float y = 152 + static_cast<float>(index) * 139;
                if (pressed && hit(481, y + 23, 324, 35)) { slider = index; }
                if (pressed && hit(489, y + 59, 116, 32))
                {
                    bool& muted = index == 0 ? settings.musicMuted : settings.effectsMuted;
                    muted = !muted;
                    settings.save();
                    clicked = true;
                }
            }
            if (slider >= 0 && (held || pressed))
            {
                Rectangle track = menuArt.optionRectangle(layout, 489, 0, 308, 1);
                int volume = static_cast<int>(std::lround(std::clamp((mouse.x - track.x) / track.width, 0.0f, 1.0f) * 100));
                int& current = slider == 0 ? settings.musicVolume : settings.effectsVolume;
                volumeChanged = volumeChanged || volume != current;
                current = volume;
            }
            else if (slider >= 0)
            {
                slider = -1;
                if (volumeChanged) { settings.save(); clicked = true; }
                volumeChanged = false;
            }
        }
        updateAudio(audio, &settings);
#if defined(__EMSCRIPTEN__)
        handleWindowShortcuts(&settings, editing >= 0);
#endif
        if (clicked) { menuClick(audio); }
        BeginDrawing();
        ClearBackground({13, 8, 6, 255});
        menuArt.drawOptions(layout, visualState, settings, help, editing, message, mouse);
        EndDrawing();
        if (close) { break; }
    }
    if (volumeChanged) { settings.save(); }
}
}

bool menuActionEnabled(int buttonIndex, int selectedSize)
{
    return (buttonIndex != 0 || selectedSize > 10) && (buttonIndex != 1 || selectedSize < 50);
}

void runProfileMenu(MenuArt& menuArt, Progression& progression, MenuAction page, MenuVisualState& visualState,
    GameSettings* settings, GameAudio* audio)
{
    Skin preview = progression.getSelectedSkin();
    float previewTime = 0.0f;
    int pressed = -1;
    bool customization = page == MenuAction::Character;
    while (!WindowShouldClose())
    {
        handleWindowShortcuts(settings);
        updateAudio(audio, settings);
        float frameTime = std::clamp(GetFrameTime(), 0.0f, 0.05f);
        visualState.elapsed += frameTime;
        previewTime += frameTime;
        MenuLayout layout = createMenuLayout(GetScreenWidth(), GetScreenHeight());
        ProfileLayout profileLayout = createProfileLayout(layout);
        int hovered = -1;
        int buttonCount = customization ? static_cast<int>(ProfileAction::Count) : 1;
        Vector2 mouse = GetMousePosition();
        for (int index = 0; index < buttonCount; index++)
        {
            if (index == static_cast<int>(ProfileAction::Confirm) && !progression.isSkinUnlocked(preview)) { continue; }
            if (CheckCollisionPointRec(mouse, profileLayout.buttons[index])) { hovered = index; }
        }
        SetMouseCursor(hovered >= 0 ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { pressed = hovered; }
        ProfileAction action = ProfileAction::None;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            if (pressed >= 0 && pressed == hovered) { action = static_cast<ProfileAction>(pressed); }
            pressed = -1;
        }
        if (customization)
        {
            if (IsKeyPressed(KEY_LEFT)) { action = ProfileAction::Previous; }
            if (IsKeyPressed(KEY_RIGHT)) { action = ProfileAction::Next; }
            if (IsKeyPressed(KEY_ENTER)) { action = ProfileAction::Confirm; }
        }
        if (IsKeyPressed(KEY_ESCAPE)) { action = ProfileAction::Back; }
        if (action == ProfileAction::Previous || action == ProfileAction::Next)
        {
            int count = static_cast<int>(Skin::CinderWarden) + 1;
            int offset = action == ProfileAction::Next ? 1 : count - 1;
            preview = static_cast<Skin>((static_cast<int>(preview) + offset) % count);
            previewTime = 0.0f;
        }
        if (action == ProfileAction::Confirm) { progression.selectSkin(preview); }
        if (action != ProfileAction::None) { menuClick(audio); }

        BeginDrawing();
        ClearBackground({13, 8, 6, 255});
        menuArt.drawProfile(layout, profileLayout, visualState, progression, page, preview, previewTime, hovered);
        EndDrawing();
        if (action == ProfileAction::Back) { return; }
    }
}

int runMainMenu(Difficulty* selectedDifficulty, Progression* progression, GameSettings* settings, GameAudio* audio)
{
    MenuArt menuArt;
    MenuVisualState visualState;
    Difficulty difficulty = Difficulty::Normal;
    int selectedSize = 25;
    int pressedButton = -1;
    int pendingSelection = -1;
    float selectionDelay = 0.0f;

    while (!WindowShouldClose())
    {
        handleWindowShortcuts(settings);
        updateAudio(audio, settings);
        float frameTime = std::clamp(GetFrameTime(), 0.0f, 0.05f);
        visualState.elapsed += frameTime;
        MenuLayout layout = createMenuLayout(GetScreenWidth(), GetScreenHeight());
        Vector2 mousePosition = GetMousePosition();
        int hoveredButton = -1;
        const int buttonCount = static_cast<int>(MenuAction::Count);
        for (int buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
        {
            if (settings == nullptr && buttonIndex >= static_cast<int>(MenuAction::Settings)) { continue; }
            if (progression == nullptr && (buttonIndex == static_cast<int>(MenuAction::Achievements)
                || buttonIndex == static_cast<int>(MenuAction::Character))) { continue; }
            if (menuActionEnabled(buttonIndex, selectedSize)
                && CheckCollisionPointRec(mousePosition, layout.buttons[buttonIndex]))
            {
                hoveredButton = buttonIndex;
            }
        }
        SetMouseCursor(hoveredButton >= 0 ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

        int activatedButton = -1;
        MenuAction profilePage = MenuAction::None;
        if (pendingSelection < 0)
        {
            if (IsKeyPressed(KEY_LEFT) && selectedSize > 10) { selectedSize--; }
            else if (IsKeyPressed(KEY_RIGHT) && selectedSize < 50) { selectedSize++; }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { pressedButton = hoveredButton; }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                if (pressedButton >= 0 && pressedButton == hoveredButton) { activatedButton = pressedButton; }
                pressedButton = -1;
            }
            if (IsKeyPressed(KEY_ENTER)) { activatedButton = static_cast<int>(MenuAction::Start); }
            if (IsKeyPressed(KEY_ESCAPE)) { activatedButton = static_cast<int>(MenuAction::Quit); }
            if (IsKeyPressed(KEY_ONE)) { activatedButton = static_cast<int>(MenuAction::Easy); }
            if (IsKeyPressed(KEY_TWO)) { activatedButton = static_cast<int>(MenuAction::Normal); }
            if (IsKeyPressed(KEY_THREE)) { activatedButton = static_cast<int>(MenuAction::Hard); }
            if (IsKeyPressed(KEY_FOUR)) { activatedButton = static_cast<int>(MenuAction::ExtraHard); }
            if (progression != nullptr && IsKeyPressed(KEY_A)) { activatedButton = static_cast<int>(MenuAction::Achievements); }
            if (progression != nullptr && IsKeyPressed(KEY_C)) { activatedButton = static_cast<int>(MenuAction::Character); }
            if (settings != nullptr && IsKeyPressed(KEY_S)) { activatedButton = static_cast<int>(MenuAction::Settings); }
            if (settings != nullptr && IsKeyPressed(KEY_SLASH)) { activatedButton = static_cast<int>(MenuAction::Help); }

            if (activatedButton >= 0 && menuActionEnabled(activatedButton, selectedSize))
            {
                menuClick(audio);
                visualState.pressAmounts[activatedButton] = 1.0f;
                if (activatedButton == static_cast<int>(MenuAction::DecreaseSize)) { selectedSize--; }
                else if (activatedButton == static_cast<int>(MenuAction::IncreaseSize)) { selectedSize++; }
                else if (activatedButton >= static_cast<int>(MenuAction::Achievements))
                {
                    profilePage = static_cast<MenuAction>(activatedButton);
                }
                else if (activatedButton >= static_cast<int>(MenuAction::Easy))
                {
                    const int presets[4] = {15, 25, 41, 49};
                    int mode = activatedButton - static_cast<int>(MenuAction::Easy);
                    difficulty = static_cast<Difficulty>(mode);
                    selectedSize = presets[mode];
                }
                else
                {
                    pendingSelection = activatedButton == static_cast<int>(MenuAction::Start) ? selectedSize : 0;
                    selectionDelay = 0.13f;
                }
            }
        }

        for (int buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
        {
            float hoverTarget = menuActionEnabled(buttonIndex, selectedSize)
                && hoveredButton == buttonIndex ? 1.0f : 0.0f;
            visualState.hoverAmounts[buttonIndex] += (hoverTarget - visualState.hoverAmounts[buttonIndex])
                * std::min(1.0f, frameTime * 12.0f);
            float pressTarget = pressedButton == buttonIndex && hoveredButton == buttonIndex
                && IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? 1.0f : 0.0f;
            visualState.pressAmounts[buttonIndex] += (pressTarget - visualState.pressAmounts[buttonIndex])
                * std::min(1.0f, frameTime * 14.0f);
        }

        BeginDrawing();
        ClearBackground({13, 8, 6, 255});
        menuArt.draw(layout, visualState, selectedSize, difficulty, progression != nullptr, settings);
        EndDrawing();

        if (profilePage != MenuAction::None)
        {
            pressedButton = -1;
            if (profilePage == MenuAction::Settings || profilePage == MenuAction::Help)
            {
                if (settings != nullptr) { runOptionsMenu(menuArt, *settings, profilePage == MenuAction::Help, visualState, audio); }
            }
            else if (progression != nullptr) { runProfileMenu(menuArt, *progression, profilePage, visualState, settings, audio); }
        }

        if (pendingSelection >= 0)
        {
            selectionDelay -= frameTime;
            if (selectionDelay <= 0.0f)
            {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
                if (selectedDifficulty != nullptr) { *selectedDifficulty = difficulty; }
                return pendingSelection;
            }
        }
    }
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    return 0;
}
