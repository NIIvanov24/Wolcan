#pragma once

#include "Maze.h"
#include "Player.h"

class GameLogic
{
public:
    GameLogic(int rowCount, int columnCount);

    bool tryMovePlayer(int rowOffset, int columnOffset);
    bool hasWon() const;
    void restart();
    const Maze& getMaze() const;
    const Player& getPlayer() const;
    int getMoveCount() const;

private:
    Maze maze;
    Player player;
    int moveCount;
};
