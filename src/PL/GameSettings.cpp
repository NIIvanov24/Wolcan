#include "PL/GameSettings.h"
#include "DAL/SaveFile.h"
#include "raylib.h"

#include <algorithm>
#include <sstream>
#include <string_view>

const char* actionName(GameAction action)
{
    const char* names[] = {"Move up", "Move down", "Move left", "Move right", "Swing hammer", "Pause / resume",
        "Restart with new seed", "Replay same seed", "Next expedition", "Return to menu", "Fullscreen"};
    int index = static_cast<int>(action);
    return index >= 0 && index < static_cast<int>(GameAction::Count) ? names[index] : "";
}

const char* keyName(int key)
{
    if (key >= KEY_A && key <= KEY_Z)
    {
        const char* letters = "A B C D E F G H I J K L M N O P Q R S T U V W X Y Z";
        return TextFormat("%c", letters[(key - KEY_A) * 2]);
    }
    if (key >= KEY_ZERO && key <= KEY_NINE) { return TextFormat("%i", key - KEY_ZERO); }
    if (key >= KEY_F1 && key <= KEY_F12) { return TextFormat("F%i", key - KEY_F1 + 1); }
    switch (key)
    {
    case KEY_UP: return "Up";
    case KEY_DOWN: return "Down";
    case KEY_LEFT: return "Left";
    case KEY_RIGHT: return "Right";
    case KEY_ESCAPE: return "Esc";
    case KEY_SPACE: return "Space";
    case KEY_ENTER: return "Enter";
    case KEY_TAB: return "Tab";
    case KEY_BACKSPACE: return "Backspace";
    case KEY_INSERT: return "Insert";
    case KEY_DELETE: return "Delete";
    case KEY_HOME: return "Home";
    case KEY_END: return "End";
    case KEY_PAGE_UP: return "Page Up";
    case KEY_PAGE_DOWN: return "Page Down";
    default: return "Unknown";
    }
}

namespace
{
bool acceptsBinding(GameAction action, int key)
{
    if (key == KEY_ESCAPE) { return action == GameAction::Menu; }
    if (action == GameAction::Fullscreen) { return key >= KEY_F1 && key <= KEY_F12; }
    // Keep the physical cheat sequence from triggering nondirectional actions.
    bool direction = action <= GameAction::Right;
    if (key == KEY_A || key == KEY_B || key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT)
    {
        return direction;
    }
    return std::string_view(keyName(key)) != "Unknown";
}

bool advanceCode(int key, const int (&sequence)[10], int& progress)
{
    if (key != sequence[progress])
    {
        // Find the longest prefix that is also the end of the buffered keys.
        int prefix = progress;
        while (prefix > 0)
        {
            int offset = progress - prefix + 1;
            bool matches = key == sequence[prefix - 1];
            for (int index = 0; index < prefix - 1; index++)
            {
                if (sequence[index] != sequence[offset + index]) { matches = false; break; }
            }
            if (matches) { break; }
            prefix--;
        }
        progress = prefix;
        return false;
    }
    progress++;
    if (progress == 10)
    {
        progress = 0;
        return true;
    }
    return false;
}
}

GameSettings::GameSettings(const std::string& path) : filePath(path)
{
    resetBindings();
    saved = load();
}

int GameSettings::key(GameAction action) const
{
    int index = static_cast<int>(action);
    return index >= 0 && index < static_cast<int>(GameAction::Count) ? keys[index] : KEY_NULL;
}

bool GameSettings::bind(GameAction action, int newKey)
{
    int index = static_cast<int>(action);
    const int count = static_cast<int>(GameAction::Count);
    if (index < 0 || index >= count || !acceptsBinding(action, newKey)) { return false; }
    for (int other = 0; other < count; other++)
    {
        if (index != other && keys[other] == newKey) { return false; }
    }
    keys[index] = newKey;
    return true;
}

void GameSettings::resetBindings()
{
    const int count = static_cast<int>(GameAction::Count);
    const int defaults[count] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_H, KEY_P,
        KEY_R, KEY_T, KEY_N, KEY_ESCAPE, KEY_F11};
    for (int index = 0; index < count; index++) { keys[index] = defaults[index]; }
}

bool GameSettings::load()
{
    if (filePath.empty()) { return true; }
    try
    {
        std::string content;
        bool exists = false;
        if (!readSaveContents(filePath, 1024, content, exists)) { return false; }
        if (!exists) { return true; }
        std::istringstream input(content);
        std::string label;
        int version = 0;
        int music = 0;
        int effects = 0;
        int muteMusic = 0;
        int muteEffects = 0;
        if (!(input >> label >> version >> music >> effects >> muteMusic >> muteEffects)
            || label != "WolcanSettings" || version != 1 || music < 0 || music > 100
            || effects < 0 || effects > 100 || muteMusic < 0 || muteMusic > 1 || muteEffects < 0 || muteEffects > 1)
        {
            return false;
        }
        const int count = static_cast<int>(GameAction::Count);
        int loaded[count] = {};
        for (int index = 0; index < count; index++)
        {
            if (!(input >> loaded[index]) || !acceptsBinding(static_cast<GameAction>(index), loaded[index])) { return false; }
            for (int other = 0; other < count; other++)
            {
                if (other < index && loaded[other] == loaded[index]) { return false; }
            }
        }
        std::string extra;
        if (input >> extra) { return false; }
        for (int index = 0; index < count; index++) { keys[index] = loaded[index]; }
        musicVolume = music;
        effectsVolume = effects;
        musicMuted = muteMusic != 0;
        effectsMuted = muteEffects != 0;
        return true;
    }
    catch (...) { return false; }
}

bool GameSettings::save()
{
    if (filePath.empty()) { saved = true; return true; }
    try
    {
        std::ostringstream output;
        output << "WolcanSettings 1" << std::endl
            << std::clamp(musicVolume, 0, 100) << ' ' << std::clamp(effectsVolume, 0, 100) << ' '
            << musicMuted << ' ' << effectsMuted << std::endl;
        const int count = static_cast<int>(GameAction::Count);
        for (int index = 0; index < count; index++) { output << keys[index] << std::endl; }
        saved = withSaveLock(filePath, [&] { return replaceSaveContents(filePath, output.str()); });
    }
    catch (...) { saved = false; }
    return saved;
}

bool GameSettings::wasSaved() const { return saved; }

bool KonamiInput::press(int pressedKey, const GameSettings& settings)
{
    const int physical[10] = {KEY_UP, KEY_UP, KEY_DOWN, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_LEFT, KEY_RIGHT, KEY_B, KEY_A};
    int up = settings.key(GameAction::Up);
    int down = settings.key(GameAction::Down);
    int left = settings.key(GameAction::Left);
    int right = settings.key(GameAction::Right);
    const int rebound[10] = {up, up, down, down, left, right, left, right, KEY_B, KEY_A};
    bool physicalMatch = advanceCode(pressedKey, physical, physicalProgress);
    bool reboundMatch = advanceCode(pressedKey, rebound, reboundProgress);
    if (!physicalMatch && !reboundMatch) { return false; }
    physicalProgress = 0;
    reboundProgress = 0;
    return true;
}
