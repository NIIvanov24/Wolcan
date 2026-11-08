#include "MainMenu.h"
#include "MenuArt.h"
#include "WindowControls.h"

#include <algorithm>

bool menuActionEnabled(int buttonIndex, int selectedSize)
{
    return (buttonIndex != 0 || selectedSize > 10) && (buttonIndex != 1 || selectedSize < 50);
}

int runMainMenu(Difficulty* selectedDifficulty)
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
        const int buttonCount = 7;
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

            if (activatedButton >= 0 && menuActionEnabled(activatedButton, selectedSize))
            {
                visualState.pressAmounts[activatedButton] = 1.0f;
                if (activatedButton == static_cast<int>(MenuAction::DecreaseSize)) { selectedSize--; }
                else if (activatedButton == static_cast<int>(MenuAction::IncreaseSize)) { selectedSize++; }
                else if (activatedButton >= static_cast<int>(MenuAction::Easy))
                {
                    difficulty = static_cast<Difficulty>(activatedButton - static_cast<int>(MenuAction::Easy));
                    selectedSize = difficulty == Difficulty::Easy ? 15 : difficulty == Difficulty::Hard ? 41 : 25;
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
        menuArt.draw(layout, visualState, selectedSize, difficulty);
        EndDrawing();

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
