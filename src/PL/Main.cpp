#include "PL/MainMenu.h"
#include "PL/GameScreen.h"
#include "PL/GameAudio.h"
#include "PL/GameSettings.h"
#include "raylib.h"

#include <exception>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <charconv>
#include <stdexcept>
#include <string>

std::string recordStoragePath()
{
#if defined(_WIN32)
    char* directory = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&directory, &length, "LOCALAPPDATA") == 0 && directory != nullptr)
    {
        std::unique_ptr<char, decltype(&std::free)> ownedDirectory(directory, &std::free);
        if (length > 1) { return std::string(directory) + "/Wolcan/best-times.txt"; }
    }
#endif
#if defined(__EMSCRIPTEN__)
    return "/user-data/best-times.txt";
#else
    return std::string(GetApplicationDirectory()) + "user-data/best-times.txt";
#endif
}

std::uint32_t requestedSeed(int argumentCount, char* arguments[])
{
    if (argumentCount == 1) { return 0; }
    if (argumentCount != 3 || std::string(arguments[1]) != "--seed")
    {
        throw std::invalid_argument("Optional launch argument: --seed <1..4294967295>");
    }
    std::string text = arguments[2];
    std::uint32_t seed = 0;
    auto result = std::from_chars(text.data(), text.data() + text.size(), seed);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || seed == 0)
    {
        throw std::invalid_argument("The seed must be a nonzero unsigned 32-bit integer.");
    }
    return seed;
}

int main(int argumentCount, char* arguments[])
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Wolcan - Volcanic Isle");
    if (!IsWindowReady())
    {
        std::cerr << "The game window could not be initialized." << std::endl;
        return 1;
    }

#if !defined(__EMSCRIPTEN__)
    int monitor = GetCurrentMonitor();
    int availableWidth = GetMonitorWidth(monitor) - 80;
    int availableHeight = GetMonitorHeight(monitor) - 80;
    int windowWidth = screenWidth < availableWidth ? screenWidth : availableWidth;
    int windowHeight = screenHeight < availableHeight ? screenHeight : availableHeight;
    if (windowWidth * 9 > windowHeight * 16) { windowWidth = windowHeight * 16 / 9; }
    else { windowHeight = windowWidth * 9 / 16; }
    if (windowWidth < 960) { windowWidth = 960; }
    if (windowHeight < 540) { windowHeight = 540; }
    SetWindowSize(windowWidth, windowHeight);
    SetWindowMinSize(960, 540);
#endif

    // Escape is handled by each screen: back to menu in-game, quit in menu.
    SetExitKey(KEY_NULL);
#if !defined(__EMSCRIPTEN__)
    SetTargetFPS(60);
#endif

    int exitCode = 0;
    try
    {
        std::uint32_t seed = requestedSeed(argumentCount, arguments);
        std::string recordFile = recordStoragePath();
        std::filesystem::path saveDirectory = std::filesystem::path(recordFile).parent_path();
        Progression progression((saveDirectory / "profile.txt").string());
        GameSettings settings((saveDirectory / "settings.txt").string());
        GameAudio audio;
        bool returnToMiniGames = false;
        while (!WindowShouldClose())
        {
            Difficulty difficulty = Difficulty::Normal;
            GameMode mode = GameMode::Expedition;
            int selectedSize = runMainMenu(&difficulty, &progression, &settings, &audio, &mode, returnToMiniGames);
            if (selectedSize == 0 || WindowShouldClose())
            {
                break;
            }

            GameLogic game(selectedSize, selectedSize, difficulty, seed, recordFile, mode);
            runGame(game, &progression, &settings, &audio);
            returnToMiniGames = mode != GameMode::Expedition;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "The game stopped: " << error.what() << std::endl;
        exitCode = 1;
    }

    CloseWindow();
    return exitCode;
}
