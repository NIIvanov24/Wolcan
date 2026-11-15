#pragma once

#include "BLL/GameLogic.h"
#include "BLL/Progression.h"

class GameSettings;
class GameAudio;
void runGame(GameLogic& game, Progression* progression = nullptr,
    const GameSettings* settings = nullptr, GameAudio* audio = nullptr);
