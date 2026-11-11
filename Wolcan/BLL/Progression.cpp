#include "Progression.h"
#include "SaveFile.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
enum class Achievement
{
    FirstDescent, VolcanoVeteran, Speedrunner, ShortestRoute, RelicHunter, Flawless,
    VolcanoSurvivor, Untouchable, AgainstAllOdds, LavaEscapeArtist, PerfectRun, Count
};

std::uint32_t achievementBit(Achievement achievement)
{
    return std::uint32_t{ 1 } << static_cast<int>(achievement);
}

bool hasAchievement(const ProfileData& profile, Achievement achievement)
{
    return (profile.achievements & achievementBit(achievement)) != 0;
}

void unlock(ProfileData& profile, Achievement achievement, bool earned)
{
    if (earned) { profile.achievements |= achievementBit(achievement); }
}

int addWithoutOverflow(int previous, int added)
{
    return previous + std::min(added, std::numeric_limits<int>::max() - previous);
}

int difficultyTier(Difficulty difficulty)
{
    switch (difficulty)
    {
    case Difficulty::Easy: return 0;
    case Difficulty::Normal: return 1;
    case Difficulty::Hard: return 2;
    case Difficulty::ExtraHard: return 3;
    }
    return -1;
}

ProfileRank saveBestRank(ProfileData& profile, const std::string& levelKey, ProfileRank rank)
{
    auto [entry, inserted] = profile.ranks.emplace(levelKey, rank);
    if (!inserted)
    {
        entry->second.bestSteps = std::min(entry->second.bestSteps, rank.bestSteps);
        entry->second.bestMedal = std::max(entry->second.bestMedal, rank.bestMedal);
    }
    return entry->second;
}

bool meetsTimeTarget(double seconds, double limit)
{
    // Medal targets and final times share the displayed hundredth of a second.
    return seconds <= limit || (seconds < limit + 0.01
        && std::round(seconds * 100.0) <= std::round(limit * 100.0));
}

bool meetsTarget(const RunSummary& run, double seconds, int steps)
{
    return meetsTimeTarget(run.seconds, seconds) && run.steps <= steps;
}

bool isValidRun(const RunSummary& run, const MedalTargets& targets)
{
    return isValidSaveKey(run.levelKey) && difficultyTier(run.difficulty) >= 0
        && run.shortestPath > 0 && run.steps >= run.shortestPath
        && std::isfinite(run.seconds) && run.seconds >= 0.0 && meetsTarget(run, targets.bronzeSeconds, targets.bronzeSteps)
        && std::isfinite(run.health) && run.health > 0.0 && run.health <= 100.0
        && std::isfinite(run.damageTaken) && run.damageTaken >= 0.0;
}
}

Progression::Progression(const std::string& filePath) : filePath(filePath)
{
    saved = filePath.empty() || readProfile(filePath, profile);
}

std::uint32_t Progression::applyChange(const std::function<void(ProfileData&)>& change, int clearIncrement, bool skinChanged)
{
    std::uint32_t newAchievements = 0;
    auto apply = [&](ProfileData& current)
    {
        std::uint32_t previouslyUnlocked = current.achievements;
        current.clears = addWithoutOverflow(current.clears, clearIncrement);
        change(current);
        unlock(current, Achievement::FirstDescent, current.clears >= 1);
        unlock(current, Achievement::VolcanoVeteran, current.ranks.size() >= 5);
        unlock(current, Achievement::RelicHunter, !current.collectibles.empty());
        newAchievements = current.achievements & ~previouslyUnlocked;
    };

    if (filePath.empty())
    {
        apply(profile);
        return newAchievements;
    }

    bool applied = false;
    saved = editProfile(filePath, [&](ProfileData& current)
    {
        if (!saved) { mergePending(current); }
        apply(current);
        profile = current;
        applied = true;
    });
    if (!applied) { apply(profile); }
    pendingClears = saved ? 0 : addWithoutOverflow(pendingClears, clearIncrement);
    pendingSkin = !saved && (pendingSkin || skinChanged);
    return newAchievements;
}

void Progression::mergePending(ProfileData& current) const
{
    // A failed write leaves progress available in memory until a later save succeeds.
    current.clears = addWithoutOverflow(current.clears, pendingClears);
    current.achievements |= profile.achievements;
    current.collectibles.insert(profile.collectibles.cbegin(), profile.collectibles.cend());
    std::for_each(profile.ranks.cbegin(), profile.ranks.cend(), [&current](const auto& entry)
    {
        saveBestRank(current, entry.first, entry.second);
    });
    if (pendingSkin) { current.selectedSkin = profile.selectedSkin; }
}

RunRewards Progression::recordFinish(const RunSummary& run)
{
    MedalTargets targets = medalTargets(run.shortestPath, run.difficulty);
    RunRewards rewards{ Medal::Bronze, Medal::Bronze, -1, 0 };
    if (!isValidRun(run, targets)) { return rewards; }

    if (meetsTarget(run, targets.goldSeconds, targets.goldSteps)) { rewards.medal = Medal::Gold; }
    else if (meetsTarget(run, targets.silverSeconds, targets.silverSteps)) { rewards.medal = Medal::Silver; }

    rewards.newAchievements = applyChange([&](ProfileData& current)
    {
        ProfileRank best = saveBestRank(current, run.levelKey, { run.steps, static_cast<int>(rewards.medal) });
        rewards.bestSteps = best.bestSteps;
        rewards.bestMedal = static_cast<Medal>(best.bestMedal);

        bool flawless = run.damageTaken == 0.0;
        bool extraHard = run.difficulty == Difficulty::ExtraHard;
        unlock(current, Achievement::Speedrunner, meetsTimeTarget(run.seconds, targets.goldSeconds));
        unlock(current, Achievement::ShortestRoute, run.steps == run.shortestPath);
        unlock(current, Achievement::Flawless, flawless);
        unlock(current, Achievement::VolcanoSurvivor, extraHard);
        unlock(current, Achievement::Untouchable, extraHard && flawless);
        unlock(current, Achievement::AgainstAllOdds, extraHard && run.health <= 10.0);
        unlock(current, Achievement::PerfectRun, extraHard && flawless && rewards.medal == Medal::Gold);
    }, 1);
    return rewards;
}

std::uint32_t Progression::recordItem(const std::string& levelKey, int row, int column)
{
    if (!isValidSaveKey(levelKey) || row < 0 || column < 0) { return 0; }
    if (saved && profile.collectibles.contains({ levelKey, row, column })) { return 0; }
    return applyChange([&](ProfileData& current) { current.collectibles.emplace(levelKey, row, column); });
}

std::uint32_t Progression::recordLavaEscape()
{
    if (saved && hasAchievement(profile, Achievement::LavaEscapeArtist)) { return 0; }
    return applyChange([](ProfileData& current) { unlock(current, Achievement::LavaEscapeArtist, true); });
}

int Progression::getAchievementCount() const { return static_cast<int>(Achievement::Count); }

AchievementInfo Progression::getAchievement(int index) const
{
    static constexpr struct AchievementText
    {
        const char* name;
        const char* description;
    } text[] = {
        { "First Descent", "Finish your first maze." },
        { "Volcano Veteran", "Finish five different mazes." },
        { "Speedrunner", "Meet a maze's Gold time target." },
        { "Shortest Route", "Finish in the fewest possible steps." },
        { "Relic Hunter", "Find a hidden collectible." },
        { "Flawless", "Finish a maze without taking damage." },
        { "Volcano Survivor", "Finish an Extra Hard maze." },
        { "Untouchable - Master of the Volcano", "Finish Extra Hard without damage." },
        { "Against All Odds", "Finish Extra Hard with 10 HP or less." },
        { "Lava Escape Artist", "Escape from a lava pit." },
        { "Perfect Run", "Earn Extra Hard Gold without damage." }
    };
    static_assert(sizeof(text) / sizeof(text[0]) == static_cast<int>(Achievement::Count));

    if (index < 0 || index >= getAchievementCount()) { return { "", "", 0, 0, false }; }
    Achievement achievement = static_cast<Achievement>(index);
    bool unlocked = hasAchievement(profile, achievement);
    int target = 1;
    int progress = unlocked ? 1 : 0;
    if (achievement == Achievement::VolcanoVeteran)
    {
        target = 5;
        progress = static_cast<int>(std::min(profile.ranks.size(), static_cast<std::size_t>(target)));
    }
    return { text[index].name, text[index].description, progress, target, unlocked };
}

int Progression::getCollectibleCount() const { return static_cast<int>(profile.collectibles.size()); }

Skin Progression::getSelectedSkin() const
{
    Skin selected = static_cast<Skin>(profile.selectedSkin);
    return isSkinUnlocked(selected) ? selected : Skin::Explorer;
}

bool Progression::isSkinUnlocked(Skin skin) const
{
    switch (skin)
    {
    case Skin::Explorer: return true;
    case Skin::LavaWalker: return hasAchievement(profile, Achievement::FirstDescent);
    case Skin::ObsidianKnight: return hasAchievement(profile, Achievement::Flawless);
    case Skin::MagmaSpirit: return getCollectibleCount() >= 3;
    case Skin::AshWanderer: return hasAchievement(profile, Achievement::VolcanoVeteran);
    case Skin::CrimsonGuardian: return hasAchievement(profile, Achievement::PerfectRun);
    }
    return false;
}

bool Progression::selectSkin(Skin skin)
{
    if (!isSkinUnlocked(skin)) { return false; }
    if (saved && static_cast<int>(skin) == profile.selectedSkin) { return true; }
    applyChange([skin](ProfileData& current) { current.selectedSkin = static_cast<int>(skin); }, 0, true);
    return true;
}

bool Progression::wasSaved() const { return saved; }

SkinInfo skinInfo(Skin skin)
{
    switch (skin)
    {
    case Skin::Explorer: return { "Explorer", "Available from the start." };
    case Skin::LavaWalker: return { "Lava Walker", "Finish your first maze." };
    case Skin::ObsidianKnight: return { "Obsidian Knight", "Finish a maze without taking damage." };
    case Skin::MagmaSpirit: return { "Magma Spirit", "Find three unique collectibles." };
    case Skin::AshWanderer: return { "Ash Wanderer", "Finish five different mazes." };
    case Skin::CrimsonGuardian: return { "Crimson Guardian", "Earn the Perfect Run achievement." };
    }
    return { "Explorer", "Available from the start." };
}

const char* skinAssetName(Skin skin)
{
    switch (skin)
    {
    case Skin::LavaWalker: return "lava-walker";
    case Skin::ObsidianKnight: return "obsidian-knight";
    case Skin::MagmaSpirit: return "magma-spirit";
    case Skin::AshWanderer: return "ash-wanderer";
    case Skin::CrimsonGuardian: return "crimson-guardian";
    case Skin::Explorer: return "explorer";
    }
    return "explorer";
}

const char* medalName(Medal medal)
{
    switch (medal)
    {
    case Medal::Gold: return "Gold";
    case Medal::Silver: return "Silver";
    case Medal::Bronze: return "Bronze";
    }
    return "Bronze";
}

MedalTargets medalTargets(int shortestPath, Difficulty difficulty)
{
    int route = std::max(1, shortestPath);
    int tier = std::max(0, difficultyTier(difficulty));
    // A route baseline scales with the exact seed. Higher tiers allow extra time and detours.
    // Bronze recognizes every successful finish with a valid time and step count.
    return {
        5.0 + route * (0.4 + tier * 0.06),
        8.0 + route * (0.7 + tier * 0.07),
        addWithoutOverflow(route, route / 12 + tier),
        addWithoutOverflow(route, route / 3 + tier * 2),
        std::numeric_limits<double>::max(),
        std::numeric_limits<int>::max()
    };
}
