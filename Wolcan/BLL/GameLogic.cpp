#include "GameLogic.h"

GameLogic::GameLogic(int rowCount, int columnCount)
    : maze(rowCount, columnCount), player(), moveCount(0)
{
}

bool GameLogic::tryMovePlayer(int rowOffset, int columnOffset)
{
    // Check exact directions before adding offsets, including extreme inputs.
    bool isVerticalMove = (rowOffset == -1 || rowOffset == 1) && columnOffset == 0;
    bool isHorizontalMove = rowOffset == 0 && (columnOffset == -1 || columnOffset == 1);
    if (hasWon() || (!isVerticalMove && !isHorizontalMove))
    {
        return false;
    }

    int nextRow = player.getRow() + rowOffset;
    int nextColumn = player.getColumn() + columnOffset;
    if (!maze.isWalkable(nextRow, nextColumn))
    {
        return false;
    }

    player.setPosition(nextRow, nextColumn);
    moveCount++;
    return true;
}

bool GameLogic::hasWon() const
{
    return player.getRow() == maze.getRowCount() - 1
        && player.getColumn() == maze.getColumnCount() - 1;
}

void GameLogic::restart()
{
    player.setPosition(0, 0);
    moveCount = 0;
}

const Maze& GameLogic::getMaze() const
{
    return maze;
}

const Player& GameLogic::getPlayer() const
{
    return player;
}

int GameLogic::getMoveCount() const
{
    return moveCount;
}
