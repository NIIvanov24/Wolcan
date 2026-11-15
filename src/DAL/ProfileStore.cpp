#include "DAL/ProfileStore.h"
#include "DAL/SaveFile.h"

#include <algorithm>
#include <filesystem>
#include <locale>
#include <sstream>
#include <utility>

namespace
{
constexpr std::uintmax_t maximumProfileBytes() { return 8 * 1024 * 1024; }

bool readEntry(const std::string& line, ProfileData& profile)
{
    if (line.size() > 192) { return false; }
    std::istringstream row(line);
    row.imbue(std::locale::classic());
    std::string kind;
    std::string levelKey;
    if (!(row >> kind >> levelKey) || !isValidSaveKey(levelKey)) { return false; }
    if (kind == "clear")
    {
        std::string extra;
        return !(row >> extra) && profile.completedLevels.insert(levelKey).second;
    }
    int first = -1;
    int second = -1;
    std::string extra;
    if (!(row >> first >> second) || (row >> extra)) { return false; }
    if (kind == "item" && first >= 0 && second >= 0)
    {
        return profile.collectibles.emplace(levelKey, first, second).second;
    }
    if (kind == "rank" && first > 0 && second >= 0 && second <= 2)
    {
        return profile.ranks.emplace(levelKey, ProfileRank{ first, second }).second;
    }
    return false;
}

bool loadProfile(const std::filesystem::path& filePath, ProfileData& profile)
{
    std::string contents;
    bool fileExists = false;
    if (!readSaveContents(filePath, maximumProfileBytes(), contents, fileExists)) { return false; }
    if (!fileExists)
    {
        profile = {};
        return true;
    }

    ProfileData parsed;
    std::istringstream lines(std::move(contents));
    std::string line;
    if (!std::getline(lines, line)) { return false; }
    if (!line.empty() && line.back() == '\r') { line.pop_back(); }
    bool legacy = line == "WolcanProfile 1";
    if ((!legacy && line != "WolcanProfile 2") || !std::getline(lines, line) || line.size() > 64) { return false; }

    std::istringstream state(line);
    state.imbue(std::locale::classic());
    std::string extra;
    int achievementCount = legacy ? 11 : ProfileData::achievementCount;
    std::uint32_t maximumAchievementMask = (std::uint32_t{ 1 } << achievementCount) - 1;
    if (!(state >> parsed.clears >> parsed.achievements >> parsed.selectedSkin)
        || (!legacy && !(state >> parsed.hammerBreaks)) || (state >> extra)
        || parsed.clears < 0 || parsed.hammerBreaks < 0 || parsed.achievements > maximumAchievementMask
        || parsed.selectedSkin < 0 || parsed.selectedSkin > (legacy ? 5 : 7))
    {
        return false;
    }

    while (std::getline(lines, line))
    {
        if (!line.empty() && line.back() == '\r') { line.pop_back(); }
        if (line == "end") { break; }
        if (!readEntry(line, parsed)) { return false; }
    }
    if (line != "end" || lines.peek() != std::char_traits<char>::eof() || lines.bad()) { return false; }
    std::for_each(parsed.ranks.cbegin(), parsed.ranks.cend(), [&parsed](const auto& entry)
    {
        parsed.completedLevels.insert(entry.first);
    });
    profile = std::move(parsed);
    return true;
}

bool saveProfile(const std::filesystem::path& filePath, const ProfileData& profile)
{
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << "WolcanProfile 2" << std::endl;
    output << profile.clears << ' ' << profile.achievements << ' ' << profile.selectedSkin << ' '
        << profile.hammerBreaks << std::endl;
    std::for_each(profile.ranks.cbegin(), profile.ranks.cend(), [&output](const auto& entry)
    {
        output << "rank " << entry.first << ' ' << entry.second.bestSteps << ' ' << entry.second.bestMedal << std::endl;
    });
    std::for_each(profile.completedLevels.cbegin(), profile.completedLevels.cend(), [&](const std::string& key)
    {
        if (!profile.ranks.contains(key)) { output << "clear " << key << std::endl; }
    });
    std::for_each(profile.collectibles.cbegin(), profile.collectibles.cend(), [&output](const auto& item)
    {
        const auto& [levelKey, row, column] = item;
        output << "item " << levelKey << ' ' << row << ' ' << column << std::endl;
    });
    output << "end" << std::endl;
    if (!output) { return false; }
    std::string contents = std::move(output).str();
    return contents.size() <= maximumProfileBytes() && replaceSaveContents(filePath, contents);
}
}

bool readProfile(const std::string& filePath, ProfileData& profile)
{
    if (!isValidSavePath(filePath)) { return false; }
    try
    {
        return loadProfile(std::filesystem::path(filePath), profile);
    }
    catch (...)
    {
        return false;
    }
}

bool editProfile(const std::string& filePath, const std::function<void(ProfileData&)>& edit)
{
    if (!isValidSavePath(filePath)) { return false; }
    try
    {
        std::filesystem::path target(filePath);
        return withSaveLock(target, [&]
        {
            ProfileData profile;
            if (!loadProfile(target, profile)) { return false; }
            edit(profile);
            return saveProfile(target, profile);
        });
    }
    catch (...)
    {
        return false;
    }
}
