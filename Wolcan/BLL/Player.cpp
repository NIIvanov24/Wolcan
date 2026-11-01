#include "Player.h"

Player::Player() : playerRow(0), playerColumn(0)
{
}

int Player::getRow() const
{
    return playerRow;
}

int Player::getColumn() const
{
    return playerColumn;
}

void Player::setPosition(int row, int column)
{
    playerRow = row;
    playerColumn = column;
}
