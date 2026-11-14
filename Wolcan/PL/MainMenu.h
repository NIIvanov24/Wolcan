#pragma once
#include "GameTypes.h"

class Progression;
class GameSettings;
class GameAudio;

int runMainMenu(Difficulty* selectedDifficulty = nullptr, Progression* progression = nullptr,
    GameSettings* settings = nullptr, GameAudio* audio = nullptr);
