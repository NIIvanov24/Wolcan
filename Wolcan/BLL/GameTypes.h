#pragma once

#include <cstdint>

enum class Difficulty { Easy, Normal, Hard, ExtraHard };
enum class HazardKind { None, Fragile, Lava, Spikes };
enum class HazardPhase { Dormant, Warning, Active, Retracting };
enum class ItemKind { None, Rune, Crystal, Relic };

struct GridPosition
{
    int row;
    int column;
};

struct CellFeature
{
    HazardKind hazard = HazardKind::None;
    HazardPhase phase = HazardPhase::Dormant;
    ItemKind item = ItemKind::None;
    int wallRowOffset = 0;
    int wallColumnOffset = 0;
    float phaseProgress = 0.0f;
};

const char* difficultyName(Difficulty difficulty);
