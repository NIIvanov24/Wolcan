#pragma once

class Maze
{
public:
    Maze(int rowCount, int columnCount);
    ~Maze();

    Maze(const Maze& other) = delete;
    Maze& operator=(const Maze& other) = delete;

    int getRowCount() const;
    int getColumnCount() const;
    bool isInside(int row, int column) const;
    bool isWalkable(int row, int column) const;
    char getCell(int row, int column) const;

private:
    int rowCount;
    int columnCount;
    char** mazeCells;

    void createStarterLayout();
    void releaseMemory();
};
