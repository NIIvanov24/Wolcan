#pragma once

class Player
{
public:
    Player();
    int getRow() const;
    int getColumn() const;
    void setPosition(int row, int column);

private:
    int playerRow;
    int playerColumn;
};
