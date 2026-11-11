#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

bool isValidSaveKey(const std::string& levelKey);
bool isValidSavePath(const std::string& filePath);
bool readSaveContents(const std::filesystem::path& filePath, std::uintmax_t maximumSize, std::string& contents, bool& fileExists);
bool ensureSaveDirectory(const std::filesystem::path& filePath);
bool replaceSaveContents(const std::filesystem::path& filePath, const std::string& contents);
bool withSaveLock(const std::filesystem::path& filePath, const std::function<bool()>& edit);
