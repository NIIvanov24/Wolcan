#pragma once
#include "BLL/GameTypes.h"

class Progression;
class GameSettings;
class GameAudio;

int runMainMenu(Difficulty* selectedDifficulty = nullptr, Progression* progression = nullptr,
    GameSettings* settings = nullptr, GameAudio* audio = nullptr, GameMode* selectedMode = nullptr,
    bool openMiniGames = false);
