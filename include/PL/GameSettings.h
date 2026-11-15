#pragma once

#include <string>

enum class GameAction { Up, Down, Left, Right, Hammer, Pause, Restart, Replay, NextLevel, Menu, Fullscreen, Count };

const char* actionName(GameAction action);
const char* keyName(int key);

class GameSettings
{
public:
    explicit GameSettings(const std::string& path = "");

    int key(GameAction action) const;
    bool bind(GameAction action, int newKey);
    void resetBindings();
    bool save();
    bool wasSaved() const;

    int musicVolume = 35;
    int effectsVolume = 65;
    bool musicMuted = false;
    bool effectsMuted = false;

private:
    int keys[static_cast<int>(GameAction::Count)] = {};
    std::string filePath;
    bool saved = true;

    bool load();
};

class KonamiInput
{
public:
    bool press(int pressedKey, const GameSettings& settings);

private:
    int physicalProgress = 0;
    int reboundProgress = 0;
};
