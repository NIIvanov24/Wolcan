#include "SaveManager.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace
{
bool isValidKey(const std::string& levelKey)
{
    if (levelKey.empty() || levelKey.size() > 128)
    {
        return false;
    }
    int characterCount = static_cast<int>(levelKey.size());
    for (int index = 0; index < characterCount; index++)
    {
        unsigned char character = static_cast<unsigned char>(levelKey[index]);
        if (character < 33 || character > 126)
        {
            return false;
        }
    }
    return true;
}

bool isValidPath(const std::string& filePath)
{
    return !filePath.empty() && filePath.find(static_cast<char>(0)) == std::string::npos;
}

bool loadRecords(const std::filesystem::path& filePath, std::map<std::string, double>& records)
{
    std::error_code fileError;
    bool fileExists = std::filesystem::exists(filePath, fileError);
    if (fileError)
    {
        return false;
    }
    if (!fileExists)
    {
        return true;
    }
    if (!std::filesystem::is_regular_file(filePath, fileError) || fileError)
    {
        return false;
    }
    std::uintmax_t fileSize = std::filesystem::file_size(filePath, fileError);
    if (fileError || fileSize > 1024 * 1024)
    {
        return false;
    }

    std::ifstream input(filePath, std::ios::binary);
    if (!input)
    {
        return false;
    }
    std::string contents(static_cast<std::size_t>(fileSize), static_cast<char>(0));
    input.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (input.gcount() != static_cast<std::streamsize>(contents.size()) || input.bad())
    {
        return false;
    }
    // Refuse a changing file rather than reading beyond the bounded snapshot.
    if (input.peek() != std::char_traits<char>::eof() || input.bad())
    {
        return false;
    }

    std::istringstream lines(contents);
    std::string line;
    while (std::getline(lines, line))
    {
        if (line.size() > 512)
        {
            continue;
        }
        std::istringstream row(line);
        row.imbue(std::locale::classic());
        std::string levelKey;
        std::string extraToken;
        double seconds = -1.0;
        if (!(row >> levelKey >> seconds) || (row >> extraToken) || !isValidKey(levelKey) || !std::isfinite(seconds) || seconds < 0.0)
        {
            continue;
        }
        auto recordPosition = records.find(levelKey);
        if (recordPosition == records.end())
        {
            if (records.size() >= 4096)
            {
                return false;
            }
            records.emplace(levelKey, seconds);
        }
        else if (seconds < recordPosition->second)
        {
            recordPosition->second = seconds;
        }
    }
    return true;
}

bool replaceRecords(const std::filesystem::path& filePath, const std::string& contents)
{
    std::random_device randomSource;
    for (int index = 0; index < 16; index++)
    {
        std::filesystem::path temporaryPath = filePath;
        temporaryPath += ".tmp-" + std::to_string(randomSource()) + "-" + std::to_string(randomSource());
#ifdef _WIN32
        HANDLE fileHandle = CreateFileW(temporaryPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            DWORD fileError = GetLastError();
            if (fileError == ERROR_FILE_EXISTS || fileError == ERROR_ALREADY_EXISTS)
            {
                continue;
            }
            return false;
        }
        DWORD writtenBytes = 0;
        bool writeSucceeded = WriteFile(fileHandle, contents.data(), static_cast<DWORD>(contents.size()), &writtenBytes, nullptr) != 0;
        writeSucceeded = writeSucceeded && writtenBytes == contents.size();
        if (writeSucceeded)
        {
            writeSucceeded = FlushFileBuffers(fileHandle) != 0;
        }
        bool closeSucceeded = CloseHandle(fileHandle) != 0;
        if (writeSucceeded && closeSucceeded && MoveFileExW(temporaryPath.c_str(), filePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0)
        {
            return true;
        }
        DeleteFileW(temporaryPath.c_str());
        return false;
#else
        int fileHandle = open(temporaryPath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fileHandle < 0)
        {
            if (errno == EEXIST)
            {
                continue;
            }
            return false;
        }
        std::size_t writtenBytes = 0;
        bool writeSucceeded = true;
        while (writtenBytes < contents.size())
        {
            ssize_t writeCount = write(fileHandle, contents.data() + writtenBytes, contents.size() - writtenBytes);
            if (writeCount <= 0)
            {
                writeSucceeded = false;
                break;
            }
            writtenBytes += static_cast<std::size_t>(writeCount);
        }
        if (writeSucceeded)
        {
            writeSucceeded = fsync(fileHandle) == 0;
        }
        bool closeSucceeded = close(fileHandle) == 0;
        if (writeSucceeded && closeSucceeded && rename(temporaryPath.c_str(), filePath.c_str()) == 0)
        {
            return true;
        }
        unlink(temporaryPath.c_str());
        return false;
#endif
    }
    return false;
}
}

double readBestTime(const std::string& filePath, const std::string& levelKey)
{
    if (!isValidKey(levelKey) || !isValidPath(filePath))
    {
        return -1.0;
    }
    try
    {
        std::map<std::string, double> records;
        if (!loadRecords(std::filesystem::path(filePath), records))
        {
            return -1.0;
        }
        auto recordPosition = records.find(levelKey);
        if (recordPosition != records.end())
        {
            return recordPosition->second;
        }
    }
    catch (...)
    {
        return -1.0;
    }
    return -1.0;
}

bool writeBestTime(const std::string& filePath, const std::string& levelKey, double seconds)
{
    if (!isValidKey(levelKey) || !isValidPath(filePath) || !std::isfinite(seconds) || seconds < 0.0)
    {
        return false;
    }
    try
    {
        std::filesystem::path targetPath(filePath);
        std::map<std::string, double> records;
        if (!loadRecords(targetPath, records))
        {
            return false;
        }
        auto recordPosition = records.find(levelKey);
        if (recordPosition != records.end() && recordPosition->second <= seconds)
        {
            return true;
        }
        if (recordPosition == records.end() && records.size() >= 4096)
        {
            return false;
        }
        records[levelKey] = seconds;
        std::ostringstream output;
        output.imbue(std::locale::classic());
        output << std::setprecision(std::numeric_limits<double>::max_digits10);
        auto outputPosition = records.cbegin();
        while (outputPosition != records.cend())
        {
            output << outputPosition->first << ' ' << outputPosition->second << std::endl;
            outputPosition++;
        }
        if (!output)
        {
            return false;
        }
        std::error_code directoryError;
        std::filesystem::path parentPath = targetPath.parent_path();
        if (!parentPath.empty())
        {
            std::filesystem::create_directories(parentPath, directoryError);
            if (directoryError)
            {
                return false;
            }
        }
        return replaceRecords(targetPath, output.str());
    }
    catch (...)
    {
        return false;
    }
}
