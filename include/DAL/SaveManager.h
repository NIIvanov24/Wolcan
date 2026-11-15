#pragma once

#include <string>

// Missing or invalid records return -1. Filesystem failures never escape.
double readBestTime(const std::string& filePath, const std::string& levelKey);

// Keep only faster finite, nonnegative times. Unchanged valid records succeed.
bool writeBestTime(const std::string& filePath, const std::string& levelKey, double seconds);
