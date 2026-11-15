#include "PL/WindowControls.h"
#include "PL/GameSettings.h"
#include "raylib.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

void handleWindowShortcuts(const GameSettings* settings, bool capturing)
{
    int key = settings == nullptr ? KEY_F11 : settings->key(GameAction::Fullscreen);
#if defined(__EMSCRIPTEN__)
    // The browser must handle fullscreen in the trusted key event itself.
    EM_ASM({ Module.wolcanFullscreenKey = $0 ? "" : "F" + ($1 - 289); }, static_cast<int>(capturing), key);
#else
    if (!capturing && IsKeyPressed(key))
    {
        // Borderless fullscreen uses the current monitor and preserves the
        // window's previous dimensions when toggled back by raylib.
        ToggleBorderlessWindowed();
    }
#endif
}
