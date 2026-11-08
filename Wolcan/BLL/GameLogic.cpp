#include "GameLogic.h"
#include "SaveManager.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

std::uint32_t nextMazeSeed(std::uint32_t previous)
{
    std::random_device entropy;
    auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    std::uint32_t seed = entropy() ^ static_cast<std::uint32_t>(ticks);
    if (seed == 0 || seed == previous) { seed = previous + 0x9e3779b9u; }
    if (seed == 0) { seed = 1; }
    return seed;
}

const char* difficultyName(Difficulty difficulty)
{
    return difficulty == Difficulty::Easy ? "EASY" : difficulty == Difficulty::Hard ? "HARD" : "NORMAL";
}

GameLogic::GameLogic(int rowCount, int columnCount, Difficulty difficulty, std::uint32_t seed, const std::string& recordFile)
    : maze(rowCount, columnCount, difficulty, seed == 0 ? nextMazeSeed(0) : seed), player(), moveCount(0), recordFile(recordFile)
{
    resetState();
}

bool GameLogic::tryMovePlayer(int rowOffset, int columnOffset)
{
    bool vertical = (rowOffset == -1 || rowOffset == 1) && columnOffset == 0;
    bool horizontal = rowOffset == 0 && (columnOffset == -1 || columnOffset == 1);
    if (hasWon() || paused || (!vertical && !horizontal)) { return false; }
    int row = player.getRow() + rowOffset;
    int column = player.getColumn() + columnOffset;
    if (!maze.isWalkable(row, column)) { return false; }
    player.setPosition(row, column);
    moveCount++;
    if (maze.visitCell(row, column)) { secretsFound++; }
    if (hasWon())
    {
        bestSeconds = bestSeconds < 0.0 ? elapsedSeconds : std::min(bestSeconds, elapsedSeconds);
        recordSaved = recordFile.empty() || writeBestTime(recordFile, getRecordKey(), elapsedSeconds);
    }
    return true;
}

bool GameLogic::hasWon() const
{
    GridPosition exit = maze.getExit();
    return player.getRow() == exit.row && player.getColumn() == exit.column;
}

void GameLogic::resetState()
{
    GridPosition start = maze.getStart();
    player.setPosition(start.row, start.column);
    maze.visitCell(start.row, start.column);
    moveCount = 0;
    elapsedSeconds = 0.0;
    secretsFound = 0;
    paused = false;
    recordSaved = true;
    if (!recordFile.empty()) { bestSeconds = readBestTime(recordFile, getRecordKey()); }
}

void GameLogic::restart(bool repeatSameMaze)
{
    std::uint32_t seed = maze.getSeed();
    if (!repeatSameMaze) { seed = nextMazeSeed(seed); bestSeconds = -1.0; }
    maze.generate(seed);
    resetState();
}

bool GameLogic::nextLevel()
{
    if (!hasWon()) { return false; }
    levelNumber++;
    restart(false);
    return true;
}

void GameLogic::updateTime(double seconds)
{
    if (!paused && !hasWon() && std::isfinite(seconds) && seconds > 0.0 && std::isfinite(elapsedSeconds + seconds))
    {
        elapsedSeconds += seconds;
    }
}

void GameLogic::setPaused(bool value) { paused = !hasWon() && value; }
bool GameLogic::isPaused() const { return paused; }
double GameLogic::getElapsedSeconds() const { return elapsedSeconds; }
double GameLogic::getBestSeconds() const { return bestSeconds; }
int GameLogic::getLevelNumber() const { return levelNumber; }
int GameLogic::getSecretsFound() const { return secretsFound; }
bool GameLogic::wasRecordSaved() const { return recordSaved; }
const Maze& GameLogic::getMaze() const { return maze; }
const Player& GameLogic::getPlayer() const { return player; }
int GameLogic::getMoveCount() const { return moveCount; }

std::string GameLogic::getRecordKey() const
{
    // A seed identifies one exact level; scores from different maps are not mixed.
    return "maze-v2-" + std::to_string(maze.getRowCount()) + "x" + std::to_string(maze.getColumnCount())
        + "-" + std::to_string(static_cast<int>(maze.getDifficulty())) + "-" + std::to_string(maze.getSeed());
}
