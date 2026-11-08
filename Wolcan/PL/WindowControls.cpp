#include "WindowControls.h"
#include "raylib.h"

void handleWindowShortcuts()
{
    if (IsKeyPressed(KEY_F11))
    {
        // Borderless fullscreen uses the current monitor and preserves the
        // window's previous dimensions when toggled back by raylib.
        ToggleBorderlessWindowed();
    }
}
