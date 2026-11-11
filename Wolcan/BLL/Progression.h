#pragma once

#include "GameTypes.h"
#include "../DAL/ProfileStore.h"

enum class Medal { Bronze, Silver, Gold };
enum class Skin { Explorer, LavaWalker, ObsidianKnight, MagmaSpirit, AshWanderer, CrimsonGuardian };

struct AchievementInfo
{
    const char* name;
    const char* description;
    int progress;
    int target;
    bool unlocked;
};

struct SkinInfo
{
    const char* name;
    const char* requirement;
};

struct RunSummary
{
    std::string levelKey;
    Difficulty difficulty;
    double seconds;
    int steps;
    int shortestPath;
    double health;
    double damageTaken;
};

struct MedalTargets
{
    double goldSeconds;
    double silverSeconds;
    int goldSteps;
    int silverSteps;
    double bronzeSeconds;
    int bronzeSteps;
};

struct RunRewards
{
    Medal medal;
    Medal bestMedal;
    int bestSteps;
    std::uint32_t newAchievements;
};

class Progression
{
public:
    explicit Progression(const std::string& filePath = "");

    RunRewards recordFinish(const RunSummary& run);
    std::uint32_t recordItem(const std::string& levelKey, int row, int column);
    std::uint32_t recordLavaEscape();
    int getAchievementCount() const;
    AchievementInfo getAchievement(int index) const;
    int getCollectibleCount() const;
    Skin getSelectedSkin() const;
    bool isSkinUnlocked(Skin skin) const;
    bool selectSkin(Skin skin);
    bool wasSaved() const;

private:
    std::string filePath;
    ProfileData profile;
    int pendingClears = 0;
    bool pendingSkin = false;
    bool saved = true;

    std::uint32_t applyChange(const std::function<void(ProfileData&)>& change, int clearIncrement = 0, bool skinChanged = false);
    void mergePending(ProfileData& current) const;
};

SkinInfo skinInfo(Skin skin);
const char* skinAssetName(Skin skin);
const char* medalName(Medal medal);
MedalTargets medalTargets(int shortestPath, Difficulty difficulty);
