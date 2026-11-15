#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>

struct ProfileRank
{
    int bestSteps;
    int bestMedal;
};

struct ProfileData
{
    static constexpr int achievementCount = 21;

    int clears = 0;
    std::uint32_t achievements = 0;
    int selectedSkin = 0;
    int hammerBreaks = 0;
    std::map<std::string, ProfileRank> ranks;
    std::set<std::string> completedLevels;
    std::set<std::tuple<std::string, int, int>> collectibles;
};

// Missing profiles start empty. Invalid or unsupported profiles are left intact.
bool readProfile(const std::string& filePath, ProfileData& profile);
bool editProfile(const std::string& filePath, const std::function<void(ProfileData&)>& edit);
