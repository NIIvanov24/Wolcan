
#include "Maze.h"

#include <stdexcept>

Maze::Maze(int rowCount, int columnCount)
    : rowCount(rowCount), columnCount(columnCount), mazeCells(nullptr)
{
    // Validate before allocating any memory.
    if (rowCount < 10 || rowCount > 50 || columnCount < 10 || columnCount > 50)
    {
        throw std::invalid_argument("Maze dimensions must be between 10 and 50.");
    }

    // Zero-initialized row pointers make partial-allocation cleanup safe.
    mazeCells = new char*[rowCount]();
    try
    {
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
        {
            mazeCells[rowIndex] = new char[columnCount];
        }
        createStarterLayout();
    }
    catch (...)
    {
        releaseMemory();
        throw;
    }
}

Maze::~Maze()
{
    releaseMemory();
}

int Maze::getRowCount() const
{
    return rowCount;
}

int Maze::getColumnCount() const
{
    return columnCount;
}

bool Maze::isInside(int row, int column) const
{
    return row >= 0 && row < rowCount && column >= 0 && column < columnCount;
}

bool Maze::isWalkable(int row, int column) const
{
    // Short-circuit evaluation prevents access outside the allocated array.
    return isInside(row, column) && mazeCells[row][column] != '#';
}

char Maze::getCell(int row, int column) const
{
    if (!isInside(row, column))
    {
        throw std::out_of_range("The requested cell is outside the maze.");
    }
    return mazeCells[row][column];
}

void Maze::createStarterLayout()
{
    // Alternating top/bottom gaps create a connected demonstration maze.
    // Leave the last column open so even and odd sizes both have an exit.
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        {
            bool isWallColumn = columnIndex % 2 == 1 && columnIndex < columnCount - 1;
            int gapRow = (columnIndex / 2) % 2 == 0 ? rowCount - 1 : 0;
            mazeCells[rowIndex][columnIndex] = isWallColumn && rowIndex != gapRow ? '#' : '.';
        }
    }
}

void Maze::releaseMemory()
{
    if (mazeCells != nullptr)
    {
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
        {
            delete[] mazeCells[rowIndex];
        }
        delete[] mazeCells;
        mazeCells = nullptr;
    }
}

