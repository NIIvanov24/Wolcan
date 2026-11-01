#pragma once

#include <string>

// Read raw level text. Interpretation and game rules belong in BLL.
std::string readMazeText(const std::string& filePath);
