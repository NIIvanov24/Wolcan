#pragma once

#include <cstdint>

enum class Difficulty { Easy, Normal, Hard, ExtraHard };
enum class GameMode { Expedition, FloorIsLava, SpikeRush };
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
    bool speedPotion = false;
};

const char* difficultyName(Difficulty difficulty);
const char* modeName(GameMode mode);
double miniGameTargetSeconds(GameMode mode, int shortestPath);
