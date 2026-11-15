#pragma once
#include "BLL/GameTypes.h"

class Maze
{
public:
    Maze(int rowCount, int columnCount, Difficulty difficulty = Difficulty::Normal, std::uint32_t seed = 1);
    ~Maze();

    Maze(const Maze& other) = delete;
    Maze& operator=(const Maze& other) = delete;

    int getRowCount() const;
    int getColumnCount() const;
    bool isInside(int row, int column) const;
    bool isWalkable(int row, int column) const;
    char getCell(int row, int column) const;
    GridPosition getStart() const;
    GridPosition getExit() const;
    Difficulty getDifficulty() const;
    std::uint32_t getSeed() const;
    int getFalseWallCount() const;
    bool visitCell(int row, int column);
    bool breakWall(int row, int column);
    void resetVisits();
    void generate(std::uint32_t seed);

private:
    int rowCount;
    int columnCount;
    char** mazeCells;
    Difficulty difficulty;
    std::uint32_t seed;
    GridPosition startPosition = {0, 0};
    GridPosition exitPosition = {0, 0};

    void releaseMemory();
};
