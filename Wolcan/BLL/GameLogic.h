#pragma once

#include "Maze.h"
#include "Player.h"
#include <memory>
#include <random>
#include <string>
#include <vector>

class GameLogic
{
public:
    GameLogic(int rowCount, int columnCount, Difficulty difficulty = Difficulty::Normal,
        std::uint32_t seed = 0, const std::string& recordFile = "");

    bool tryMovePlayer(int rowOffset, int columnOffset);
    bool hasWon() const;
    void restart(bool repeatSameMaze = false);
    bool nextLevel();
    void updateTime(double seconds);
    void setPaused(bool paused);
    bool isPaused() const;
    double getElapsedSeconds() const;
    double getBestSeconds() const;
    int getLevelNumber() const;
    int getSecretsFound() const;
    bool wasRecordSaved() const;
    std::string getRecordKey() const;
    const Maze& getMaze() const;
    const Player& getPlayer() const;
    int getMoveCount() const;
    CellFeature getCellFeature(int row, int column) const;
    double getHealth() const;
    double getDamageTaken() const;
    bool isDying() const;
    float getDeathProgress() const;
    unsigned int getAttemptRevision() const;
    float getMovementMultiplier() const;
    int getShortestPath() const;
    int getItemCount() const;
    int getItemsCollected() const;
    int getLavaEscapes() const;
    GridPosition getLastCollectedPosition() const;
    bool isInLava() const;

private:
    template <typename Cell>
    using CellGrid = std::unique_ptr<std::unique_ptr<Cell[]>[]>;

    struct TileState
    {
        CellFeature feature;
        double collapseAt = -1.0;
        double spikeOffset = 0.0;
        double lastSpikeHit = -1.0;
        std::uint32_t spikeSeed = 0;
    };

    struct SpikeTiming
    {
        HazardPhase phase;
        double startsAt;
        double endsAt;
    };

    static constexpr double maxHealth = 100.0;
    static constexpr double fragileWarningSeconds = 0.30;
    static constexpr double deathAnimationSeconds = 0.80;

    Maze maze;
    Player player;
    int moveCount;
    double elapsedSeconds = 0.0;
    double bestSeconds = -1.0;
    bool paused = false;
    int levelNumber = 1;
    int secretsFound = 0;
    bool recordSaved = true;
    std::string recordFile;
    CellGrid<TileState> tiles;
    int shortestPath = 0;
    int itemCount = 0;
    int itemsCollected = 0;
    int lavaEscapes = 0;
    GridPosition lastCollectedPosition = {-1, -1};
    unsigned int attemptRevision = 0;
    double health = maxHealth;
    double damageTaken = 0.0;
    double regenerationTime = 0.0;
    double respawnProtectionUntil = 0.0;
    double spikeProtectionUntil = 0.0;
    double deathTime = 0.0;

    template <typename Cell>
    CellGrid<Cell> makeGrid(const Cell& initial) const;
    CellGrid<int> getDistances(GridPosition start, bool includeSecrets, bool avoidHazards = false) const;
    void generateFeatures();
    void placeHazards(std::mt19937& generator, const CellGrid<int>& ordinaryDistances);
    SpikeTiming getSpikeTiming(const TileState& tile) const;
    void trySpikeHit(TileState& tile, const SpikeTiming& timing);
    void takeDamage(double amount);
    void regenerate(double seconds);
    void advanceContact(double until);
    void resetState(bool respawn = false);
};
