#pragma once

#include <cstdint>

enum class Difficulty { Easy, Normal, Hard };

struct GridPosition
{
    int row;
    int column;
};

const char* difficultyName(Difficulty difficulty);
