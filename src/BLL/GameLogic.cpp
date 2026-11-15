#include "BLL/GameLogic.h"
#include "DAL/SaveManager.h"

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

bool isAdjacentOffset(int rowOffset, int columnOffset)
{
    bool vertical = (rowOffset == -1 || rowOffset == 1) && columnOffset == 0;
    bool horizontal = rowOffset == 0 && (columnOffset == -1 || columnOffset == 1);
    return vertical || horizontal;
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

const char* modeName(GameMode mode)
{
    if (mode == GameMode::FloorIsLava) { return "The Floor Is Lava"; }
    if (mode == GameMode::SpikeRush) { return "Spike Rush"; }
    return "Expedition";
}

double miniGameTargetSeconds(GameMode mode, int shortestPath)
{
    if (mode == GameMode::FloorIsLava) { return std::max(8.0, 2.0 + shortestPath * 0.24); }
    if (mode == GameMode::SpikeRush) { return std::max(10.0, 3.0 + shortestPath * 0.36); }
    return 0.0;
}

GameLogic::GameLogic(int rowCount, int columnCount, Difficulty difficulty, std::uint32_t seed,
    const std::string& recordFile, GameMode mode)
    : maze(rowCount, columnCount, difficulty, seed == 0 ? nextMazeSeed(0) : seed), mode(mode), player(),
    moveCount(0), recordFile(recordFile)
{
    resetState();
}

bool GameLogic::tryMovePlayer(int rowOffset, int columnOffset)
{
    if (hasWon() || paused || isDying() || !isAdjacentOffset(rowOffset, columnOffset)) { return false; }
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
    if (tile.feature.speedPotion)
    {
        tile.feature.speedPotion = false;
        boostUntil = elapsedSeconds + boostDurationSeconds;
    }
    if (tile.feature.hazard == HazardKind::Fragile && tile.collapseAt < 0.0)
    {
        triggerCollapse({row, column});
    }
    if (leftLava && !isInLava())
    {
        lavaEscapes++;
        regenerationTime = 0.0;
    }
    if (tile.feature.hazard == HazardKind::Spikes) { trySpikeHit(tile, getSpikeTiming(tile)); }
    if (hasWon() && !assisted)
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

bool GameLogic::grantHammer()
{
    if (mode != GameMode::Expedition || hammerGranted || paused || isDying() || hasWon()) { return false; }
    hammerGranted = true;
    hammerUses = 10;
    return true;
}

int GameLogic::getHammerUses() const { return hammerUses; }
bool GameLogic::isAssisted() const { return assisted; }

bool GameLogic::canBreakWall(int row, int column) const
{
    return hammerUses > 0 && !paused && !isDying() && !hasWon()
        && row > 0 && row < maze.getRowCount() - 1 && column > 0 && column < maze.getColumnCount() - 1
        && isAdjacentOffset(row - player.getRow(), column - player.getColumn())
        && maze.getCell(row, column) == '#' && !tiles[row][column].spikeAnchor;
}

bool GameLogic::tryBreakWall(int rowOffset, int columnOffset)
{
    if (!isAdjacentOffset(rowOffset, columnOffset)) { return false; }
    int row = player.getRow() + rowOffset;
    int column = player.getColumn() + columnOffset;
    if (!canBreakWall(row, column) || !maze.breakWall(row, column)) { return false; }
    hammerUses--;
    assisted = true;
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

std::vector<GridPosition> GameLogic::getOrdinaryRoute() const
{
    CellGrid<int> distancesToExit = getDistances(maze.getExit(), false);
    GridPosition current = maze.getStart();
    std::vector<GridPosition> route = {current};
    while (distancesToExit[current.row][current.column] > 0)
    {
        int nextDistance = distancesToExit[current.row][current.column] - 1;
        auto neighbors = neighborsOf(current);
        for (int index = 0; index < static_cast<int>(neighbors.size()); index++)
        {
            GridPosition next = neighbors[index];
            if (!maze.isInside(next.row, next.column) || distancesToExit[next.row][next.column] != nextDistance) { continue; }
            current = next;
            route.push_back(current);
            break;
        }
    }
    return route;
}

void GameLogic::scheduleLavaWave(std::mt19937& generator, const CellGrid<int>& ordinaryDistances,
    const std::vector<GridPosition>& route)
{
    // Place potions in separate stretches of the race, including a final one near the exit.
    int routeLength = static_cast<int>(route.size()) - 1;
    int potionCount = std::min(3, routeLength - 1);
    for (int index = 0; index < potionCount; index++)
    {
        int first = 1 + index * (routeLength - 1) / potionCount;
        int last = (index + 1) * (routeLength - 1) / potionCount;
        if (index == potionCount - 1) { first = std::max(first, last - 7); }
        GridPosition position = route[std::uniform_int_distribution<int>(first, last)(generator)];
        tiles[position.row][position.column].feature.speedPotion = true;
    }

    GridPosition exit = maze.getExit();
    std::uniform_real_distribution<double> jitter(0.0, 0.12);
    for (int row = 0; row < maze.getRowCount(); row++)
    {
        for (int column = 0; column < maze.getColumnCount(); column++)
        {
            int distance = ordinaryDistances[row][column];
            TileState& tile = tiles[row][column];
            // The wave advances along corridors while leaving dry pockets, the potions and both gateways.
            if (distance <= 0 || distance % 8 == 0 || (row == exit.row && column == exit.column)
                || tile.feature.speedPotion) { continue; }
            tile.feature.hazard = HazardKind::Fragile;
            tile.collapseAt = 2.6 + 0.17 * distance + jitter(generator);
        }
    }
}

void GameLogic::placeSpike(std::mt19937& generator, GridPosition cell, GridPosition wall)
{
    TileState& tile = tiles[cell.row][cell.column];
    tile.feature.hazard = HazardKind::Spikes;
    tile.feature.wallRowOffset = wall.row - cell.row;
    tile.feature.wallColumnOffset = wall.column - cell.column;
    tile.spikeSeed = static_cast<std::uint32_t>(generator());
    tile.spikeOffset = std::uniform_real_distribution<double>(0.0, mode == GameMode::SpikeRush ? 1.4 : 2.5)(generator);
    tiles[wall.row][wall.column].spikeAnchor = true;
}

void GameLogic::placeRushSpikes(std::mt19937& generator, const CellGrid<int>& ordinaryDistances,
    const std::vector<GridPosition>& route)
{
    CellGrid<bool> reserved = makeGrid(false);
    GridPosition start = maze.getStart();
    GridPosition exit = maze.getExit();
    auto placeGate = [this, &generator, &reserved, start, exit](GridPosition cell)
    {
        if (reserved[cell.row][cell.column] || gridDistance(cell, start) <= 1 || gridDistance(cell, exit) <= 1) { return false; }
        auto neighbors = neighborsOf(cell);
        GridPosition walls[4] = {};
        int wallCount = 0;
        for (int direction = 0; direction < static_cast<int>(neighbors.size()); direction++)
        {
            GridPosition next = neighbors[direction];
            if (maze.isInside(next.row, next.column) && maze.getCell(next.row, next.column) == '#')
            {
                walls[wallCount++] = next;
            }
        }
        if (wallCount == 0) { return false; }
        placeSpike(generator, cell, walls[std::uniform_int_distribution<int>(0, wallCount - 1)(generator)]);
        for (int direction = 0; direction < static_cast<int>(neighbors.size()); direction++)
        {
            GridPosition next = neighbors[direction];
            if (maze.isInside(next.row, next.column)) { reserved[next.row][next.column] = true; }
        }
        return true;
    };

    // Every route spike has a safe floor on both sides where the player can wait.
    int routeSize = static_cast<int>(route.size());
    int nextGateIndex = 0;
    for (int index = 0; index < routeSize; index++)
    {
        if (index >= nextGateIndex && placeGate(route[index]))
        {
            nextGateIndex = index + 1 + std::uniform_int_distribution<int>(2, 4)(generator);
        }
    }
    for (int index = 0; index < routeSize; index++)
    {
        GridPosition cell = route[index];
        reserved[cell.row][cell.column] = true;
    }
    std::vector<GridPosition> branches;
    int rows = maze.getRowCount();
    int columns = maze.getColumnCount();
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            if (ordinaryDistances[row][column] > 0 && !reserved[row][column]) { branches.push_back({row, column}); }
        }
    }
    std::shuffle(branches.begin(), branches.end(), generator);
    int remaining = rows * columns / 18;
    int branchCount = static_cast<int>(branches.size());
    for (int index = 0; index < branchCount; index++)
    {
        if (remaining == 0) { break; }
        if (placeGate(branches[index])) { remaining--; }
    }
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
    CellGrid<bool> eligible = makeGrid(false);
    int candidateCount = static_cast<int>(candidates.size());
    for (int index = 0; index < candidateCount; index++)
    {
        GridPosition cell = candidates[index];
        eligible[cell.row][cell.column] = true;
    }
    int hazardTarget = 1 + rows * columns / 165;
    int fragileCount = 0;
    int spikeCount = 0;
    for (int candidateIndex = 0; candidateIndex < static_cast<int>(candidates.size()); candidateIndex++)
    {
        if (fragileCount >= hazardTarget && spikeCount >= hazardTarget) { break; }
        GridPosition cell = candidates[candidateIndex];
        if (reserved[cell.row][cell.column] || tiles[cell.row][cell.column].feature.hazard != HazardKind::None) { continue; }
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
        if (wallCount > 0 && spikeCount <= fragileCount && spikeCount < hazardTarget)
        {
            GridPosition wall = walls[std::uniform_int_distribution<int>(0, wallCount - 1)(generator)];
            placeSpike(generator, cell, wall);
            spikeCount++;
        }
        else if (fragileCount < hazardTarget)
        {
            reserved[escape.row][escape.column] = true;
            placeFragileTrap(generator, cell, fragileCount + 1, eligible, reserved);
            fragileCount++;
        }
        else { continue; }
        reserved[escape.row][escape.column] = true;
    }
}

void GameLogic::placeFragileTrap(std::mt19937& generator, GridPosition cell, int group,
    const CellGrid<bool>& eligible, const CellGrid<bool>& reserved)
{
    // Grow a connected group away from its reserved dry escape.
    std::vector<GridPosition> members = {cell};
    tiles[cell.row][cell.column].collapseGroup = group;
    tiles[cell.row][cell.column].feature.hazard = HazardKind::Fragile;
    int target = std::uniform_int_distribution<int>(2, 4)(generator);
    for (int index = 0; index < target - 1; index++)
    {
        std::vector<GridPosition> frontier;
        int memberCount = static_cast<int>(members.size());
        for (int member = 0; member < memberCount; member++)
        {
            auto neighbors = neighborsOf(members[member]);
            const int directionCount = 4;
            for (int direction = 0; direction < directionCount; direction++)
            {
                GridPosition next = neighbors[direction];
                if (maze.isInside(next.row, next.column) && eligible[next.row][next.column]
                    && !reserved[next.row][next.column] && tiles[next.row][next.column].feature.hazard == HazardKind::None)
                {
                    frontier.push_back(next);
                }
            }
        }
        if (frontier.empty()) { break; }
        int last = static_cast<int>(frontier.size()) - 1;
        GridPosition next = frontier[std::uniform_int_distribution<int>(0, last)(generator)];
        tiles[next.row][next.column].collapseGroup = group;
        tiles[next.row][next.column].feature.hazard = HazardKind::Fragile;
        members.push_back(next);
    }
}

void GameLogic::triggerCollapse(GridPosition cell)
{
    TileState& entry = tiles[cell.row][cell.column];
    int group = entry.collapseGroup;
    entry.collapseAt = elapsedSeconds + fragileWarningSeconds;
    std::vector<GridPosition> queue = {cell};
    for (int index = 0; index < static_cast<int>(queue.size()); index++)
    {
        GridPosition current = queue[index];
        auto neighbors = neighborsOf(current);
        const int count = 4;
        for (int direction = 0; direction < count; direction++)
        {
            GridPosition next = neighbors[direction];
            if (!maze.isInside(next.row, next.column)) { continue; }
            TileState& tile = tiles[next.row][next.column];
            if (tile.collapseGroup != group || tile.collapseAt >= 0.0) { continue; }
            tile.collapseAt = tiles[current.row][current.column].collapseAt + collapseSpreadSeconds;
            queue.push_back(next);
        }
    }
}

void GameLogic::generateFeatures()
{
    int rows = maze.getRowCount();
    int columns = maze.getColumnCount();
    tiles = makeGrid(TileState{});
    GridPosition start = maze.getStart();
    GridPosition exit = maze.getExit();
    std::mt19937 generator(maze.getSeed() ^ (0x8f4d62a1u + static_cast<std::uint32_t>(maze.getDifficulty())));
    if (mode != GameMode::Expedition)
    {
        CellGrid<int> ordinaryDistances = getDistances(start, false);
        shortestPath = ordinaryDistances[exit.row][exit.column];
        std::vector<GridPosition> route = getOrdinaryRoute();
        if (mode == GameMode::FloorIsLava) { scheduleLavaWave(generator, ordinaryDistances, route); }
        if (mode == GameMode::SpikeRush) { placeRushSpikes(generator, ordinaryDistances, route); }
        itemCount = 0;
        return;
    }
    CellGrid<int> distances = getDistances(start, true);
    shortestPath = distances[exit.row][exit.column];
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
    assisted = false;
    GridPosition start = maze.getStart();
    player.setPosition(start.row, start.column);
    maze.visitCell(start.row, start.column);
    generateFeatures();
    moveCount = 0;
    elapsedSeconds = 0.0;
    secretsFound = 0;
    health = maxHealth;
    damageTaken = 0.0;
    spikeDamageTaken = 0.0;
    boostUntil = 0.0;
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
    if (!respawn && !recordFile.empty())
    {
        double saved = readBestTime(recordFile, getRecordKey());
        if (mode == GameMode::Expedition || bestSeconds < 0.0 || (saved >= 0.0 && saved < bestSeconds))
        {
            bestSeconds = saved;
        }
    }
}

void GameLogic::restart(bool repeatSameMaze)
{
    if (repeatSameMaze)
    {
        if (assisted) { maze.generate(maze.getSeed()); }
        else { maze.resetVisits(); }
    }
    else
    {
        maze.generate(nextMazeSeed(maze.getSeed()));
        if (mode == GameMode::Expedition) { bestSeconds = -1.0; }
    }
    resetState();
}

bool GameLogic::nextLevel()
{
    if (mode != GameMode::Expedition || !hasWon()) { return false; }
    levelNumber++;
    restart(false);
    return true;
}

GameLogic::SpikeTiming GameLogic::getSpikeTiming(const TileState& tile) const
{
    bool rush = mode == GameMode::SpikeRush;
    double cycleSeconds = rush ? 2.35 : 3.85;
    double warningSeconds = rush ? 0.32 : 0.44;
    double activeSeconds = rush ? 0.50 : 0.54;
    double retractSeconds = rush ? 0.15 : 0.22;
    double firstWarning = rush ? 0.22 : 0.50;
    double jitterSeconds = rush ? 0.94 : 1.75;
    double cycle = std::floor(std::max(0.0, elapsedSeconds - tile.spikeOffset) / cycleSeconds);
    auto warningStart = [&tile, cycleSeconds, firstWarning, jitterSeconds](double cycleIndex)
    {
        auto wrapped = static_cast<std::uint32_t>(std::fmod(cycleIndex, 4294967296.0));
        double jitter = static_cast<double>(mixSpikeCycle(tile.spikeSeed + wrapped)) / 4294967296.0;
        return tile.spikeOffset + cycleIndex * cycleSeconds + firstWarning + jitterSeconds * jitter;
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
        double warningSeconds = mode == GameMode::FloorIsLava ? floorWarningSeconds : fragileWarningSeconds;
        if (elapsedSeconds >= tile.collapseAt)
        {
            feature.hazard = HazardKind::Lava;
            feature.phase = HazardPhase::Active;
            feature.phaseProgress = static_cast<float>(std::clamp((elapsedSeconds - tile.collapseAt) / 0.28, 0.0, 1.0));
        }
        else if (elapsedSeconds >= tile.collapseAt - warningSeconds - 0.000000001)
        {
            feature.phase = HazardPhase::Warning;
            feature.phaseProgress = static_cast<float>(std::clamp(1.0 - (tile.collapseAt - elapsedSeconds) / warningSeconds,
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

double GameLogic::takeDamage(double amount)
{
    double applied = std::min(health, amount);
    health -= applied;
    damageTaken += applied;
    regenerationTime = 0.0;
    return applied;
}

void GameLogic::trySpikeHit(TileState& tile, const SpikeTiming& timing)
{
    if (timing.phase == HazardPhase::Active && elapsedSeconds >= std::max(respawnProtectionUntil, spikeProtectionUntil)
        && tile.lastSpikeHit != timing.startsAt)
    {
        tile.lastSpikeHit = timing.startsAt;
        spikeProtectionUntil = elapsedSeconds + 0.90;
        spikeDamageTaken += takeDamage(12.0);
    }
}

void GameLogic::regenerate(double seconds)
{
    if (health >= maxHealth) { return; }
    regenerationTime += seconds;
    double intervals = std::floor((regenerationTime + 0.000000001) / regenerationInterval);
    health = std::min(maxHealth, health + 4.0 * intervals);
    regenerationTime = health >= maxHealth ? 0.0 : std::max(0.0, regenerationTime - intervals * regenerationInterval);
}

void GameLogic::advanceContact(double until)
{
    TileState& tile = tiles[player.getRow()][player.getColumn()];
    if (tile.feature.hazard == HazardKind::Fragile && tile.collapseAt >= 0.0)
    {
        if (elapsedSeconds < tile.collapseAt)
        {
            double warningEnd = std::min(until, tile.collapseAt);
            regenerate(warningEnd - elapsedSeconds);
            elapsedSeconds = warningEnd;
        }
        if (elapsedSeconds >= until) { return; }
        double exposureAt = std::max(elapsedSeconds, respawnProtectionUntil);
        double fatalAt = exposureAt + health / lavaDamagePerSecond;
        regenerationTime = 0.0;
        if (fatalAt <= until + 0.000000001)
        {
            elapsedSeconds = std::min(fatalAt, until);
            takeDamage(health);
        }
        else
        {
            takeDamage(std::max(0.0, until - exposureAt) * lavaDamagePerSecond);
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
    if (assisted) { maze.generate(maze.getSeed()); }
    else { maze.resetVisits(); }
    resetState(true);
    elapsedSeconds = std::max(0.0, seconds - remainingDeathTime);
}

void GameLogic::setPaused(bool value) { paused = !hasWon() && value; }
bool GameLogic::isPaused() const { return paused; }
double GameLogic::getElapsedSeconds() const { return elapsedSeconds; }
double GameLogic::getBestSeconds() const { return bestSeconds; }
GameMode GameLogic::getMode() const { return mode; }
double GameLogic::getRushTargetSeconds() const { return miniGameTargetSeconds(mode, shortestPath); }
int GameLogic::getLevelNumber() const { return levelNumber; }
int GameLogic::getSecretsFound() const { return secretsFound; }
bool GameLogic::wasRecordSaved() const { return recordSaved; }
const Maze& GameLogic::getMaze() const { return maze; }
const Player& GameLogic::getPlayer() const { return player; }
int GameLogic::getMoveCount() const { return moveCount; }
double GameLogic::getHealth() const { return health; }
double GameLogic::getDamageTaken() const { return damageTaken; }
double GameLogic::getSpikeDamageTaken() const { return spikeDamageTaken; }
double GameLogic::getBoostSeconds() const
{
    double remaining = boostUntil - elapsedSeconds;
    return remaining > 0.000000001 ? std::min(remaining, boostDurationSeconds) : 0.0;
}
bool GameLogic::isDying() const { return health <= 0.0; }
float GameLogic::getDeathProgress() const { return isDying() ? static_cast<float>(deathTime / deathAnimationSeconds) : 0.0f; }
unsigned int GameLogic::getAttemptRevision() const { return attemptRevision; }
float GameLogic::getMovementMultiplier() const
{
    float speed = getBoostSeconds() > 0.0 ? 1.4f : 1.0f;
    return isInLava() ? speed * 0.55f : speed;
}
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
    std::string dimensions = std::to_string(maze.getRowCount()) + "x" + std::to_string(maze.getColumnCount());
    if (mode == GameMode::FloorIsLava) { return "mini-v1-floor-" + dimensions; }
    if (mode == GameMode::SpikeRush) { return "mini-v1-spikes-" + dimensions; }

    // A seed identifies one exact level; scores from different maps are not mixed.
    std::string version = maze.getDifficulty() == Difficulty::ExtraHard ? "maze-v3-" : "maze-v2-";
    return version + dimensions
        + "-" + std::to_string(static_cast<int>(maze.getDifficulty())) + "-" + std::to_string(maze.getSeed());
}
