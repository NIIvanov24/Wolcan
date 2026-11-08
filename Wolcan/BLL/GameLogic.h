#pragma once

#include "Maze.h"
#include "Player.h"
#include <string>

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

private:
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

    void resetState();
};
