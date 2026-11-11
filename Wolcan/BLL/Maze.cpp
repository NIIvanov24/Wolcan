#include "Maze.h"

#include <algorithm>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

struct DistanceGrid
{
    std::unique_ptr<std::unique_ptr<int[]>[]> cells;

    DistanceGrid(int rows, int columns) : cells(std::make_unique<std::unique_ptr<int[]>[]>(rows))
    {
        for (int rowIndex = 0; rowIndex < rows; rowIndex++)
        {
            cells[rowIndex] = std::make_unique<int[]>(columns);
            for (int columnIndex = 0; columnIndex < columns; columnIndex++)
            {
                cells[rowIndex][columnIndex] = -1;
            }
        }
    }
    int* operator[](int row) const { return cells[row].get(); }
};

bool ordinaryPassage(const Maze& maze, int row, int column)
{
    return maze.isInside(row, column) && maze.getCell(row, column) != '#'
        && maze.getCell(row, column) != 'X' && maze.getCell(row, column) != '?';
}

DistanceGrid mazeDistances(const Maze& maze, GridPosition start)
{
    DistanceGrid distances(maze.getRowCount(), maze.getColumnCount());
    std::vector<GridPosition> queue;
    queue.push_back(start);
    distances[start.row][start.column] = 0;
    int rowOffsets[4] = {-1, 1, 0, 0};
    int columnOffsets[4] = {0, 0, -1, 1};
    const int directionCount = 4;
    for (int queueIndex = 0; queueIndex < static_cast<int>(queue.size()); queueIndex++)
    {
        GridPosition current = queue[queueIndex];
        for (int directionIndex = 0; directionIndex < directionCount; directionIndex++)
        {
            int row = current.row + rowOffsets[directionIndex];
            int column = current.column + columnOffsets[directionIndex];
            if (ordinaryPassage(maze, row, column) && distances[row][column] < 0)
            {
                distances[row][column] = distances[current.row][current.column] + 1;
                queue.push_back({row, column});
            }
        }
    }
    return distances;
}

int randomChoice(std::mt19937& generator, int count)
{
    return std::uniform_int_distribution<int>(0, count - 1)(generator);
}

bool separatesCorridors(const Maze& maze, int row, int column)
{
    if (row <= 0 || column <= 0 || row >= maze.getRowCount() - 1 || column >= maze.getColumnCount() - 1
        || maze.getCell(row, column) != '#') { return false; }
    bool horizontal = ordinaryPassage(maze, row, column - 1) && ordinaryPassage(maze, row, column + 1);
    bool vertical = ordinaryPassage(maze, row - 1, column) && ordinaryPassage(maze, row + 1, column);
    return horizontal != vertical;
}

Maze::Maze(int rowCount, int columnCount, Difficulty difficulty, std::uint32_t seed)
    : rowCount(rowCount), columnCount(columnCount), mazeCells(nullptr), difficulty(difficulty), seed(seed)
{
    if (rowCount < 10 || rowCount > 50 || columnCount < 10 || columnCount > 50
        || static_cast<int>(difficulty) < 0 || static_cast<int>(difficulty) > static_cast<int>(Difficulty::ExtraHard))
    {
        throw std::invalid_argument("Maze dimensions must be 10..50 and difficulty must be valid.");
    }
    mazeCells = new char*[rowCount]();
    try
    {
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
        {
            mazeCells[rowIndex] = new char[columnCount];
        }
        generate(seed);
    }
    catch (...) { releaseMemory(); throw; }
}

Maze::~Maze() { releaseMemory(); }
int Maze::getRowCount() const { return rowCount; }
int Maze::getColumnCount() const { return columnCount; }
GridPosition Maze::getStart() const { return startPosition; }
GridPosition Maze::getExit() const { return exitPosition; }
Difficulty Maze::getDifficulty() const { return difficulty; }
std::uint32_t Maze::getSeed() const { return seed; }

bool Maze::isInside(int row, int column) const
{
    return row >= 0 && row < rowCount && column >= 0 && column < columnCount;
}

bool Maze::isWalkable(int row, int column) const
{
    return isInside(row, column) && mazeCells[row][column] != '#' && mazeCells[row][column] != 'X';
}

char Maze::getCell(int row, int column) const
{
    if (!isInside(row, column)) { throw std::out_of_range("Cell is outside the maze."); }
    return mazeCells[row][column];
}

int Maze::getFalseWallCount() const
{
    int count = 0;
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        {
            if (mazeCells[rowIndex][columnIndex] == '?' || mazeCells[rowIndex][columnIndex] == '+') { count++; }
        }
    }
    return count;
}

bool Maze::visitCell(int row, int column)
{
    if (!isWalkable(row, column)) { return false; }
    if (mazeCells[row][column] == '?') { mazeCells[row][column] = '+'; return true; }
    if (mazeCells[row][column] == '.') { mazeCells[row][column] = ','; }
    return false;
}

void Maze::resetVisits()
{
    for (int row = 0; row < rowCount; row++)
    {
        for (int column = 0; column < columnCount; column++)
        {
            if (mazeCells[row][column] == ',') { mazeCells[row][column] = '.'; }
            else if (mazeCells[row][column] == '+') { mazeCells[row][column] = '?'; }
        }
    }
}

void Maze::generate(std::uint32_t newSeed)
{
    seed = newSeed;
    std::mt19937 generator(seed ^ (static_cast<std::uint32_t>(difficulty) * 0x9e3779b9u));
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++) { mazeCells[rowIndex][columnIndex] = '#'; }
    }
    int lastRow = rowCount - 2;
    int lastColumn = columnCount - 2;
    if (lastRow % 2 == 0) { lastRow--; }
    if (lastColumn % 2 == 0) { lastColumn--; }
    int rowOffsets[4] = {-1, 1, 0, 0};
    int columnOffsets[4] = {0, 0, -1, 1};
    const int directionCount = 4;
    std::vector<GridPosition> active;
    active.push_back({1, 1});
    mazeCells[1][1] = '.';
    while (!active.empty())
    {
        int chosenIndex = static_cast<int>(active.size()) - 1;
        // Easy grows a broad frontier; Hard uses a deep backtracking stack.
        if (difficulty == Difficulty::Easy || (difficulty == Difficulty::Normal && randomChoice(generator, 100) < 40))
        {
            chosenIndex = randomChoice(generator, static_cast<int>(active.size()));
        }
        GridPosition current = active[chosenIndex];
        GridPosition candidates[4] = {};
        int candidateCount = 0;
        for (int directionIndex = 0; directionIndex < directionCount; directionIndex++)
        {
            int row = current.row + rowOffsets[directionIndex] * 2;
            int column = current.column + columnOffsets[directionIndex] * 2;
            if (row >= 1 && row <= lastRow && column >= 1 && column <= lastColumn && mazeCells[row][column] == '#')
            {
                candidates[candidateCount] = {row, column};
                candidateCount++;
            }
        }
        if (candidateCount == 0) { active.erase(active.begin() + chosenIndex); }
        else
        {
            GridPosition next = candidates[randomChoice(generator, candidateCount)];
            mazeCells[(current.row + next.row) / 2][(current.column + next.column) / 2] = '.';
            mazeCells[next.row][next.column] = '.';
            active.push_back(next);
        }
    }

    std::vector<GridPosition> walls;
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        {
            if (separatesCorridors(*this, rowIndex, columnIndex)) { walls.push_back({rowIndex, columnIndex}); }
        }
    }
    std::shuffle(walls.begin(), walls.end(), generator);
    int nodeCount = ((lastRow + 1) / 2) * ((lastColumn + 1) / 2);
    int secretTarget = difficulty == Difficulty::Easy ? std::max(1, nodeCount / 100)
        : difficulty == Difficulty::Normal ? std::max(2, nodeCount / 55) : std::max(3, nodeCount / 25);
    secretTarget = std::min(secretTarget, static_cast<int>(walls.size()));
    for (int secretIndex = 0; secretIndex < secretTarget; secretIndex++)
    {
        GridPosition wall = walls[secretIndex];
        mazeCells[wall.row][wall.column] = '?';
    }
    int loopTarget = difficulty == Difficulty::Easy ? nodeCount / 5 : difficulty == Difficulty::Normal ? nodeCount / 12 : nodeCount / 80;
    if (difficulty == Difficulty::ExtraHard) { loopTarget = std::max(1, loopTarget); }
    int openedLoops = 0;
    for (int wallIndex = 0; wallIndex < static_cast<int>(walls.size()); wallIndex++)
    {
        GridPosition wall = walls[wallIndex];
        if (openedLoops < loopTarget && separatesCorridors(*this, wall.row, wall.column))
        {
            mazeCells[wall.row][wall.column] = '.';
            openedLoops++;
        }
    }
    // Braid Easy's dead ends; Normal keeps some, Hard retains its backtracking traps.
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        {
            if (rowIndex % 2 == 0 || columnIndex % 2 == 0 || rowIndex > lastRow || columnIndex > lastColumn
                || difficulty == Difficulty::Hard || difficulty == Difficulty::ExtraHard
                || !ordinaryPassage(*this, rowIndex, columnIndex)) { continue; }
            int neighborCount = 0;
            GridPosition candidates[4] = {};
            int candidateCount = 0;
            for (int directionIndex = 0; directionIndex < directionCount; directionIndex++)
            {
                int row = rowIndex + rowOffsets[directionIndex];
                int column = columnIndex + columnOffsets[directionIndex];
                if (ordinaryPassage(*this, row, column)) { neighborCount++; }
                if (separatesCorridors(*this, row, column)) { candidates[candidateCount++] = {row, column}; }
            }
            if (neighborCount == 1 && candidateCount > 0
                && (difficulty == Difficulty::Easy || randomChoice(generator, 3) == 0))
            {
                GridPosition gap = candidates[randomChoice(generator, candidateCount)];
                mazeCells[gap.row][gap.column] = '.';
            }
        }
    }

    int orientation = randomChoice(generator, 4);
    GridPosition interior = orientation < 2 ? GridPosition{1 + randomChoice(generator, (lastRow + 1) / 2) * 2, orientation == 0 ? 1 : lastColumn}
        : GridPosition{orientation == 2 ? 1 : lastRow, 1 + randomChoice(generator, (lastColumn + 1) / 2) * 2};
    startPosition = orientation < 2 ? GridPosition{interior.row, orientation == 0 ? 0 : columnCount - 1}
        : GridPosition{orientation == 2 ? 0 : rowCount - 1, interior.column};
    auto openGateway = [this](GridPosition inside, GridPosition outside)
    {
        mazeCells[inside.row][inside.column] = '.';
        while (inside.row != outside.row || inside.column != outside.column)
        {
            if (inside.row != outside.row) { inside.row += inside.row < outside.row ? 1 : -1; }
            else { inside.column += inside.column < outside.column ? 1 : -1; }
            mazeCells[inside.row][inside.column] = '.';
        }
    };
    openGateway(interior, startPosition);
    DistanceGrid distances = mazeDistances(*this, startPosition);
    std::vector<GridPosition> exits;
    int exitCount = orientation < 2 ? (lastRow + 1) / 2 : (lastColumn + 1) / 2;
    for (int exitIndex = 0; exitIndex < exitCount; exitIndex++)
    {
        exits.push_back(orientation < 2 ? GridPosition{exitIndex * 2 + 1, orientation == 0 ? lastColumn : 1}
            : GridPosition{orientation == 2 ? lastRow : 1, exitIndex * 2 + 1});
    }
    std::sort(exits.begin(), exits.end(), [&distances](GridPosition first, GridPosition second)
        { return distances[first.row][first.column] < distances[second.row][second.column]; });
    int exitIndex = static_cast<int>(exits.size()) - 1;
    if (difficulty == Difficulty::Easy) { exitIndex = randomChoice(generator, std::max(1, exitCount / 3)); }
    else if (difficulty == Difficulty::Normal) { exitIndex = exitCount / 2 + randomChoice(generator, exitCount - exitCount / 2); }
    GridPosition exitInterior = exits[exitIndex];
    exitPosition = orientation < 2 ? GridPosition{exitInterior.row, orientation == 0 ? columnCount - 1 : 0}
        : GridPosition{orientation == 2 ? rowCount - 1 : 0, exitInterior.column};
    openGateway(exitInterior, exitPosition);

    // Extra Hard uses dynamic hazards instead of boulders that could erase a bypass.
    int obstacleTarget = difficulty == Difficulty::Easy || difficulty == Difficulty::ExtraHard
        ? 0 : difficulty == Difficulty::Normal ? 1 : 3;
    if (obstacleTarget == 0) { return; }
    // Add boulders only when the ordinary passage graph stays fully connected.
    int obstacles = 0;
    std::vector<GridPosition> floors;
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        {
            if (rowIndex > 0 && rowIndex < rowCount - 1 && columnIndex > 0 && columnIndex < columnCount - 1
                && mazeCells[rowIndex][columnIndex] == '.') { floors.push_back({rowIndex, columnIndex}); }
        }
    }
    std::shuffle(floors.begin(), floors.end(), generator);
    int attemptCount = std::min(24, static_cast<int>(floors.size()));
    for (int attemptIndex = 0; attemptIndex < attemptCount; attemptIndex++)
    {
        if (obstacles >= obstacleTarget) { break; }
        GridPosition floor = floors[attemptIndex];
        bool touchesSecret = false;
        for (int directionIndex = 0; directionIndex < directionCount; directionIndex++)
        {
            if (mazeCells[floor.row + rowOffsets[directionIndex]][floor.column + columnOffsets[directionIndex]] == '?') { touchesSecret = true; }
        }
        if (touchesSecret) { continue; }
        mazeCells[floor.row][floor.column] = 'X';
        DistanceGrid remaining = mazeDistances(*this, startPosition);
        bool connected = true;
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
        {
            for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
            {
                if (ordinaryPassage(*this, rowIndex, columnIndex) && remaining[rowIndex][columnIndex] < 0) { connected = false; }
            }
        }
        if (connected) { obstacles++; }
        else { mazeCells[floor.row][floor.column] = '.'; }
    }
}

void Maze::releaseMemory()
{
    if (mazeCells != nullptr)
    {
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) { delete[] mazeCells[rowIndex]; }
        delete[] mazeCells;
        mazeCells = nullptr;
    }
}
