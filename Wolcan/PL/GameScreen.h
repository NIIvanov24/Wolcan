#pragma once

#include "GameLogic.h"
#include "Progression.h"

class GameSettings;
class GameAudio;
void runGame(GameLogic& game, Progression* progression = nullptr,
    const GameSettings* settings = nullptr, GameAudio* audio = nullptr);
