
#include "MazeLoader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::string readMazeText(const std::string& filePath)
{
    std::ifstream levelFile(filePath);
    if (!levelFile.is_open())
    {
        throw std::runtime_error("Cannot open the level file: " + filePath);
    }

    std::ostringstream mazeText;
    mazeText << levelFile.rdbuf();
    if (levelFile.bad() || mazeText.bad())
    {
        throw std::runtime_error("Cannot read the level file: " + filePath);
    }
    return mazeText.str();
}
