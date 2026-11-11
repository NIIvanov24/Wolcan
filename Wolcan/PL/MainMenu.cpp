#include "MainMenu.h"
#include "MenuArt.h"
#include "WindowControls.h"

#include <algorithm>

bool menuActionEnabled(int buttonIndex, int selectedSize)
{
    return (buttonIndex != 0 || selectedSize > 10) && (buttonIndex != 1 || selectedSize < 50);
}

void runProfileMenu(MenuArt& menuArt, Progression& progression, MenuAction page, MenuVisualState& visualState)
{
    Skin preview = progression.getSelectedSkin();
    float previewTime = 0.0f;
    int pressed = -1;
    bool customization = page == MenuAction::Character;
    while (!WindowShouldClose())
    {
        handleWindowShortcuts();
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
            int count = static_cast<int>(Skin::CrimsonGuardian) + 1;
            int offset = action == ProfileAction::Next ? 1 : count - 1;
            preview = static_cast<Skin>((static_cast<int>(preview) + offset) % count);
            previewTime = 0.0f;
        }
        if (action == ProfileAction::Confirm) { progression.selectSkin(preview); }

        BeginDrawing();
        ClearBackground({13, 8, 6, 255});
        menuArt.drawProfile(layout, profileLayout, visualState, progression, page, preview, previewTime, hovered);
        EndDrawing();
        if (action == ProfileAction::Back) { return; }
    }
}

int runMainMenu(Difficulty* selectedDifficulty, Progression* progression)
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
        handleWindowShortcuts();
        float frameTime = std::clamp(GetFrameTime(), 0.0f, 0.05f);
        visualState.elapsed += frameTime;
        MenuLayout layout = createMenuLayout(GetScreenWidth(), GetScreenHeight());
        Vector2 mousePosition = GetMousePosition();
        int hoveredButton = -1;
        const int buttonCount = progression != nullptr ? static_cast<int>(MenuAction::Count)
            : static_cast<int>(MenuAction::Achievements);
        for (int buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
        {
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

            if (activatedButton >= 0 && menuActionEnabled(activatedButton, selectedSize))
            {
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
        menuArt.draw(layout, visualState, selectedSize, difficulty, progression != nullptr);
        EndDrawing();

        if (profilePage != MenuAction::None && progression != nullptr)
        {
            pressedButton = -1;
            runProfileMenu(menuArt, *progression, profilePage, visualState);
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
