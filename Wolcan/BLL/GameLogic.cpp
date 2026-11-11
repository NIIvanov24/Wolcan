#include "GameLogic.h"
#include "SaveManager.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <random>

namespace
{
std::array<GridPosition, 4> neighborsOf(GridPosition cell)
{
    return {{{cell.row - 1, cell.column}, {cell.row + 1, cell.column},
        {cell.row, cell.column - 1}, {cell.row, cell.column + 1}}};
}

int gridDistance(GridPosition first, GridPosition second)
{
    return std::abs(first.row - second.row) + std::abs(first.column - second.column);
}

std::uint32_t mixSpikeCycle(std::uint32_t value)
{
    // Integer mixing gives each spike an independent, repeatable interval per cycle.
    value = (value ^ (value >> 16)) * 0x7feb352du;
    value = (value ^ (value >> 15)) * 0x846ca68bu;
    return value ^ (value >> 16);
}
}

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
    if (difficulty == Difficulty::ExtraHard) { return "EXTRA HARD"; }
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
    if (hasWon() || paused || isDying() || (!vertical && !horizontal)) { return false; }
    int row = player.getRow() + rowOffset;
    int column = player.getColumn() + columnOffset;
    if (!maze.isWalkable(row, column)) { return false; }
    bool leftLava = isInLava();
    player.setPosition(row, column);
    moveCount++;
    if (maze.visitCell(row, column)) { secretsFound++; }
    TileState& tile = tiles[row][column];
    if (tile.feature.item != ItemKind::None)
    {
        tile.feature.item = ItemKind::None;
        itemsCollected++;
        lastCollectedPosition = {row, column};
    }
    if (tile.feature.hazard == HazardKind::Fragile && tile.collapseAt < 0.0)
    {
        tile.collapseAt = elapsedSeconds + fragileWarningSeconds;
    }
    if (leftLava && !isInLava())
    {
        lavaEscapes++;
        regenerationTime = 0.0;
    }
    if (tile.feature.hazard == HazardKind::Spikes) { trySpikeHit(tile, getSpikeTiming(tile)); }
    if (hasWon())
    {
        bestSeconds = bestSeconds < 0.0 ? elapsedSeconds : std::min(bestSeconds, elapsedSeconds);
        if (!recordFile.empty())
        {
            std::string key = getRecordKey();
            recordSaved = writeBestTime(recordFile, key, elapsedSeconds);
            if (recordSaved)
            {
                double storedTime = readBestTime(recordFile, key);
                if (storedTime >= 0.0) { bestSeconds = std::min(bestSeconds, storedTime); }
            }
        }
    }
    return true;
}

bool GameLogic::hasWon() const
{
    GridPosition exit = maze.getExit();
    return player.getRow() == exit.row && player.getColumn() == exit.column;
}

template <typename Cell>
GameLogic::CellGrid<Cell> GameLogic::makeGrid(const Cell& initial) const
{
    int rows = maze.getRowCount();
    int columns = maze.getColumnCount();
    CellGrid<Cell> grid = std::make_unique<std::unique_ptr<Cell[]>[]>(rows);
    for (int row = 0; row < rows; row++)
    {
        grid[row] = std::make_unique<Cell[]>(columns);
        for (int column = 0; column < columns; column++) { grid[row][column] = initial; }
    }
    return grid;
}

GameLogic::CellGrid<int> GameLogic::getDistances(GridPosition start, bool includeSecrets, bool avoidHazards) const
{
    CellGrid<int> distances = makeGrid(-1);
    GridPosition exit = maze.getExit();
    std::vector<GridPosition> queue = {start};
    distances[start.row][start.column] = 0;
    for (int queueIndex = 0; queueIndex < static_cast<int>(queue.size()); queueIndex++)
    {
        GridPosition current = queue[queueIndex];
        if (avoidHazards && current.row == exit.row && current.column == exit.column) { continue; }
        int nextDistance = distances[current.row][current.column] + 1;
        auto neighbors = neighborsOf(current);
        for (int neighborIndex = 0; neighborIndex < static_cast<int>(neighbors.size()); neighborIndex++)
        {
            GridPosition next = neighbors[neighborIndex];
            if (!maze.isWalkable(next.row, next.column)) { continue; }
            if (!includeSecrets && maze.getCell(next.row, next.column) == '?') { continue; }
            if (distances[next.row][next.column] >= 0
                || (avoidHazards && tiles[next.row][next.column].feature.hazard != HazardKind::None)) { continue; }
            distances[next.row][next.column] = nextDistance;
            queue.push_back(next);
        }
    }
    return distances;
}

void GameLogic::placeHazards(std::mt19937& generator, const CellGrid<int>& ordinaryDistances)
{
    GridPosition start = maze.getStart();
    GridPosition exit = maze.getExit();
    CellGrid<bool> reserved = makeGrid(false);
    GridPosition route = exit;
    // Protect an ordinary start-to-exit route before placing optional traps.
    while (true)
    {
        reserved[route.row][route.column] = true;
        int distance = ordinaryDistances[route.row][route.column];
        if (distance <= 0) { break; }
        auto neighbors = neighborsOf(route);
        for (int neighborIndex = 0; neighborIndex < static_cast<int>(neighbors.size()); neighborIndex++)
        {
            GridPosition next = neighbors[neighborIndex];
            if (maze.isInside(next.row, next.column) && ordinaryDistances[next.row][next.column] == distance - 1)
            {
                route = next;
                break;
            }
        }
    }

    std::vector<GridPosition> candidates;
    int rows = maze.getRowCount();
    int columns = maze.getColumnCount();
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            GridPosition cell = {row, column};
            if (!reserved[row][column] && ordinaryDistances[row][column] > 0
                && gridDistance(cell, start) > 2 && gridDistance(cell, exit) > 2)
            {
                candidates.push_back(cell);
            }
        }
    }
    std::shuffle(candidates.begin(), candidates.end(), generator);
    int hazardTarget = 1 + rows * columns / 165;
    int fragileCount = 0;
    int spikeCount = 0;
    for (int candidateIndex = 0; candidateIndex < static_cast<int>(candidates.size()); candidateIndex++)
    {
        if (fragileCount >= hazardTarget && spikeCount >= hazardTarget) { break; }
        GridPosition cell = candidates[candidateIndex];
        if (reserved[cell.row][cell.column]) { continue; }
        auto neighbors = neighborsOf(cell);
        GridPosition walls[4] = {};
        int wallCount = 0;
        GridPosition escape = {-1, -1};
        for (int neighborIndex = 0; neighborIndex < static_cast<int>(neighbors.size()); neighborIndex++)
        {
            GridPosition next = neighbors[neighborIndex];
            if (!maze.isInside(next.row, next.column)) { continue; }
            if (maze.getCell(next.row, next.column) == '#') { walls[wallCount++] = next; }
            if (ordinaryDistances[next.row][next.column] >= 0 && tiles[next.row][next.column].feature.hazard == HazardKind::None
                && (escape.row < 0 || reserved[next.row][next.column]))
            {
                escape = next;
            }
        }
        if (escape.row < 0) { continue; }
        TileState& tile = tiles[cell.row][cell.column];
        if (wallCount > 0 && spikeCount <= fragileCount && spikeCount < hazardTarget)
        {
            GridPosition wall = walls[std::uniform_int_distribution<int>(0, wallCount - 1)(generator)];
            tile.feature.hazard = HazardKind::Spikes;
            tile.feature.wallRowOffset = wall.row - cell.row;
            tile.feature.wallColumnOffset = wall.column - cell.column;
            tile.spikeSeed = static_cast<std::uint32_t>(generator());
            tile.spikeOffset = std::uniform_real_distribution<double>(0.0, 2.5)(generator);
            spikeCount++;
        }
        else if (fragileCount < hazardTarget)
        {
            tile.feature.hazard = HazardKind::Fragile;
            fragileCount++;
        }
        else { continue; }
        reserved[escape.row][escape.column] = true;
    }
}

void GameLogic::generateFeatures()
{
    int rows = maze.getRowCount();
    int columns = maze.getColumnCount();
    tiles = makeGrid(TileState{});
    GridPosition start = maze.getStart();
    GridPosition exit = maze.getExit();
    CellGrid<int> distances = getDistances(start, true);
    shortestPath = distances[exit.row][exit.column];
    std::mt19937 generator(maze.getSeed() ^ (0x8f4d62a1u + static_cast<std::uint32_t>(maze.getDifficulty())));
    if (maze.getDifficulty() == Difficulty::ExtraHard) { placeHazards(generator, getDistances(start, false)); }
    CellGrid<int> safeDistances = getDistances(start, true, true);
    CellGrid<int> distancesToExit = getDistances(exit, true);
    std::vector<GridPosition> candidates;
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            if (safeDistances[row][column] > 2 && (row != exit.row || column != exit.column) && maze.getCell(row, column) != '?')
            {
                candidates.push_back({row, column});
            }
        }
    }
    std::shuffle(candidates.begin(), candidates.end(), generator);
    // Prefer off-route discoveries while keeping a fallback for compact mazes.
    std::stable_partition(candidates.begin(), candidates.end(), [this, &distances, &distancesToExit](GridPosition cell)
        { return distances[cell.row][cell.column] + distancesToExit[cell.row][cell.column] > shortestPath; });
    ItemKind kinds[3] = {ItemKind::Rune, ItemKind::Crystal, ItemKind::Relic};
    int kindOffset = std::uniform_int_distribution<int>(0, 2)(generator);
    itemCount = std::min(2 + rows * columns / 400, static_cast<int>(candidates.size()));
    for (int itemIndex = 0; itemIndex < itemCount; itemIndex++)
    {
        GridPosition cell = candidates[itemIndex];
        tiles[cell.row][cell.column].feature.item = kinds[(kindOffset + itemIndex) % 3];
    }
}

void GameLogic::resetState(bool respawn)
{
    GridPosition start = maze.getStart();
    player.setPosition(start.row, start.column);
    maze.visitCell(start.row, start.column);
    generateFeatures();
    moveCount = 0;
    elapsedSeconds = 0.0;
    secretsFound = 0;
    health = maxHealth;
    damageTaken = 0.0;
    regenerationTime = 0.0;
    respawnProtectionUntil = respawn ? 1.25 : 0.0;
    spikeProtectionUntil = 0.0;
    deathTime = 0.0;
    itemsCollected = 0;
    lavaEscapes = 0;
    lastCollectedPosition = {-1, -1};
    attemptRevision++;
    paused = false;
    recordSaved = true;
    if (!respawn && !recordFile.empty()) { bestSeconds = readBestTime(recordFile, getRecordKey()); }
}

void GameLogic::restart(bool repeatSameMaze)
{
    if (repeatSameMaze) { maze.resetVisits(); }
    else
    {
        maze.generate(nextMazeSeed(maze.getSeed()));
        bestSeconds = -1.0;
    }
    resetState();
}

bool GameLogic::nextLevel()
{
    if (!hasWon()) { return false; }
    levelNumber++;
    restart(false);
    return true;
}

GameLogic::SpikeTiming GameLogic::getSpikeTiming(const TileState& tile) const
{
    constexpr double cycleSeconds = 5.0;
    constexpr double warningSeconds = 0.55;
    constexpr double activeSeconds = 0.62;
    constexpr double retractSeconds = 0.30;
    double cycle = std::floor(std::max(0.0, elapsedSeconds - tile.spikeOffset) / cycleSeconds);
    auto warningStart = [&tile](double cycleIndex)
    {
        auto wrapped = static_cast<std::uint32_t>(std::fmod(cycleIndex, 4294967296.0));
        double jitter = static_cast<double>(mixSpikeCycle(tile.spikeSeed + wrapped)) / 4294967296.0;
        return tile.spikeOffset + cycleIndex * cycleSeconds + 0.75 + 2.5 * jitter;
    };
    double warningAt = warningStart(cycle);
    double activeAt = warningAt + warningSeconds;
    double retractAt = activeAt + activeSeconds;
    double dormantAt = retractAt + retractSeconds;
    if (elapsedSeconds < warningAt) { return {HazardPhase::Dormant, 0.0, warningAt}; }
    if (elapsedSeconds < activeAt) { return {HazardPhase::Warning, warningAt, activeAt}; }
    if (elapsedSeconds < retractAt) { return {HazardPhase::Active, activeAt, retractAt}; }
    if (elapsedSeconds < dormantAt) { return {HazardPhase::Retracting, retractAt, dormantAt}; }
    return {HazardPhase::Dormant, dormantAt, warningStart(cycle + 1.0)};
}

CellFeature GameLogic::getCellFeature(int row, int column) const
{
    if (!maze.isInside(row, column)) { return {}; }
    const TileState& tile = tiles[row][column];
    CellFeature feature = tile.feature;
    if (feature.hazard == HazardKind::Fragile && tile.collapseAt >= 0.0)
    {
        if (elapsedSeconds >= tile.collapseAt)
        {
            feature.hazard = HazardKind::Lava;
            feature.phase = HazardPhase::Active;
            feature.phaseProgress = 1.0f;
        }
        else
        {
            feature.phase = HazardPhase::Warning;
            feature.phaseProgress = static_cast<float>(std::clamp(1.0 - (tile.collapseAt - elapsedSeconds) / fragileWarningSeconds,
                0.0, 1.0));
        }
    }
    if (feature.hazard == HazardKind::Spikes)
    {
        SpikeTiming timing = getSpikeTiming(tile);
        feature.phase = timing.phase;
        if (timing.phase != HazardPhase::Dormant)
        {
            feature.phaseProgress = static_cast<float>((elapsedSeconds - timing.startsAt) / (timing.endsAt - timing.startsAt));
        }
    }
    return feature;
}

void GameLogic::takeDamage(double amount)
{
    double applied = std::min(health, amount);
    health -= applied;
    damageTaken += applied;
    regenerationTime = 0.0;
}

void GameLogic::trySpikeHit(TileState& tile, const SpikeTiming& timing)
{
    if (timing.phase == HazardPhase::Active && elapsedSeconds >= std::max(respawnProtectionUntil, spikeProtectionUntil)
        && tile.lastSpikeHit != timing.startsAt)
    {
        tile.lastSpikeHit = timing.startsAt;
        spikeProtectionUntil = elapsedSeconds + 0.90;
        takeDamage(12.0);
    }
}

void GameLogic::regenerate(double seconds)
{
    if (health >= maxHealth) { return; }
    regenerationTime += seconds;
    double intervals = std::floor((regenerationTime + 0.000000001) / 2.0);
    health = std::min(maxHealth, health + 4.0 * intervals);
    regenerationTime = health >= maxHealth ? 0.0 : std::max(0.0, regenerationTime - intervals * 2.0);
}

void GameLogic::advanceContact(double until)
{
    TileState& tile = tiles[player.getRow()][player.getColumn()];
    if (tile.feature.hazard == HazardKind::Fragile)
    {
        if (elapsedSeconds < tile.collapseAt)
        {
            double warningEnd = std::min(until, tile.collapseAt);
            regenerate(warningEnd - elapsedSeconds);
            elapsedSeconds = warningEnd;
        }
        if (elapsedSeconds >= until) { return; }
        double exposureAt = std::max(elapsedSeconds, respawnProtectionUntil);
        double fatalAt = exposureAt + health / 4.0;
        regenerationTime = 0.0;
        if (fatalAt <= until + 0.000000001)
        {
            elapsedSeconds = std::min(fatalAt, until);
            takeDamage(health);
        }
        else
        {
            takeDamage(std::max(0.0, until - exposureAt) * 4.0);
            elapsedSeconds = until;
        }
        return;
    }
    if (tile.feature.hazard == HazardKind::Spikes)
    {
        // Advance at each contact boundary so a long frame still sees the full activation.
        while (elapsedSeconds < until)
        {
            SpikeTiming timing = getSpikeTiming(tile);
            double next = std::min(until, timing.endsAt);
            if (next <= elapsedSeconds) { next = until; }
            if (timing.phase == HazardPhase::Active)
            {
                trySpikeHit(tile, timing);
                if (isDying()) { return; }
                regenerationTime = 0.0;
                double protectedUntil = std::max(respawnProtectionUntil, spikeProtectionUntil);
                if (tile.lastSpikeHit != timing.startsAt && protectedUntil > elapsedSeconds)
                {
                    next = std::min(next, protectedUntil);
                }
            }
            else { regenerate(next - elapsedSeconds); }
            elapsedSeconds = next;
        }
        trySpikeHit(tile, getSpikeTiming(tile));
        return;
    }
    regenerate(until - elapsedSeconds);
    elapsedSeconds = until;
}

void GameLogic::updateTime(double seconds)
{
    if (paused || hasWon() || !std::isfinite(seconds) || seconds <= 0.0 || !std::isfinite(elapsedSeconds + seconds)) { return; }
    if (!isDying())
    {
        double until = elapsedSeconds + seconds;
        advanceContact(until);
        if (!isDying()) { return; }
        seconds = until - elapsedSeconds;
    }
    double remainingDeathTime = deathAnimationSeconds - deathTime;
    if (seconds + 0.000000001 < remainingDeathTime)
    {
        deathTime += seconds;
        return;
    }
    maze.resetVisits();
    resetState(true);
    elapsedSeconds = std::max(0.0, seconds - remainingDeathTime);
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
double GameLogic::getHealth() const { return health; }
double GameLogic::getDamageTaken() const { return damageTaken; }
bool GameLogic::isDying() const { return health <= 0.0; }
float GameLogic::getDeathProgress() const { return isDying() ? static_cast<float>(deathTime / deathAnimationSeconds) : 0.0f; }
unsigned int GameLogic::getAttemptRevision() const { return attemptRevision; }
float GameLogic::getMovementMultiplier() const { return isInLava() ? 0.55f : 1.0f; }
int GameLogic::getShortestPath() const { return shortestPath; }
int GameLogic::getItemCount() const { return itemCount; }
int GameLogic::getItemsCollected() const { return itemsCollected; }
int GameLogic::getLavaEscapes() const { return lavaEscapes; }
GridPosition GameLogic::getLastCollectedPosition() const { return lastCollectedPosition; }
bool GameLogic::isInLava() const
{
    const TileState& tile = tiles[player.getRow()][player.getColumn()];
    return tile.feature.hazard == HazardKind::Fragile && tile.collapseAt >= 0.0 && elapsedSeconds >= tile.collapseAt;
}

std::string GameLogic::getRecordKey() const
{
    // A seed identifies one exact level; scores from different maps are not mixed.
    return "maze-v2-" + std::to_string(maze.getRowCount()) + "x" + std::to_string(maze.getColumnCount())
        + "-" + std::to_string(static_cast<int>(maze.getDifficulty())) + "-" + std::to_string(maze.getSeed());
}
