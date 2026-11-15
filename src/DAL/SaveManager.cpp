#include "DAL/SaveManager.h"
#include "DAL/SaveFile.h"

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

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace
{
#if defined(__EMSCRIPTEN__)
EM_ASYNC_JS(int, acquireWebSaveLock, (const char* path), {
    if (!navigator.locks) return 1;
    const key = "wolcan:" + UTF8ToString(path);
    return await new Promise((ready) => {
        navigator.locks.request(key, () => new Promise((release) => {
            if (!Module.wolcanSaveLocks) Module.wolcanSaveLocks = new Map();
            Module.wolcanSaveLocks.set(key, release);
            ready(1);
        })).catch(() => ready(0));
    });
});

class SaveLock
{
public:
    explicit SaveLock(const std::filesystem::path& filePath) : path(filePath.string()),
        locked(acquireWebSaveLock(path.c_str()) != 0) {}

    ~SaveLock()
    {
        if (!locked) { return; }
        EM_ASM({
            const key = "wolcan:" + UTF8ToString($0);
            const release = Module.wolcanSaveLocks?.get(key);
            Module.wolcanSaveLocks?.delete(key);
            if (release) release();
        }, path.c_str());
    }

    SaveLock(const SaveLock&) = delete;
    SaveLock& operator=(const SaveLock&) = delete;
    bool isLocked() const { return locked; }

private:
    std::string path;
    bool locked;
};
#else
class SaveLock
{
public:
    explicit SaveLock(std::filesystem::path filePath)
    {
        // The lock must survive atomic replacement of the actual save file.
        filePath += ".lock";
#ifdef _WIN32
        handle = CreateFileW(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle != INVALID_HANDLE_VALUE)
        {
            OVERLAPPED region{};
            locked = LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &region) != 0;
        }
#else
        descriptor = open(filePath.c_str(), O_CREAT | O_RDWR, 0600);
        if (descriptor >= 0) { locked = flock(descriptor, LOCK_EX) == 0; }
#endif
    }

    ~SaveLock()
    {
#ifdef _WIN32
        if (locked)
        {
            OVERLAPPED region{};
            UnlockFileEx(handle, 0, 1, 0, &region);
        }
        if (handle != INVALID_HANDLE_VALUE) { CloseHandle(handle); }
#else
        if (descriptor >= 0) { close(descriptor); }
#endif
    }

    SaveLock(const SaveLock&) = delete;
    SaveLock& operator=(const SaveLock&) = delete;
    bool isLocked() const { return locked; }

private:
    bool locked = false;
#ifdef _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
#else
    int descriptor = -1;
#endif
};
#endif
}

bool withSaveLock(const std::filesystem::path& filePath, const std::function<bool()>& edit)
{
#if !defined(__EMSCRIPTEN__)
    if (!ensureSaveDirectory(filePath)) { return false; }
#endif
    SaveLock lock(filePath);
    return lock.isLocked() && edit();
}

bool isValidSaveKey(const std::string& levelKey)
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

bool isValidSavePath(const std::string& filePath)
{
    return !filePath.empty() && filePath.find(static_cast<char>(0)) == std::string::npos;
}

bool readSaveContents(const std::filesystem::path& filePath, std::uintmax_t maximumSize, std::string& contents, bool& fileExists)
{
#if defined(__EMSCRIPTEN__)
    std::string path = filePath.string();
    int size = EM_ASM_INT({
        try {
            const value = localStorage.getItem("wolcan:" + UTF8ToString($0));
            return value === null ? -1 : lengthBytesUTF8(value);
        } catch (error) { return -2; }
    }, path.c_str());
    fileExists = size >= 0;
    if (size < 0) { return size == -1; }
    if (static_cast<std::uintmax_t>(size) > maximumSize) { return false; }
    contents.resize(static_cast<std::size_t>(size) + 1);
    int copied = EM_ASM_INT({
        try {
            const value = localStorage.getItem("wolcan:" + UTF8ToString($0));
            if (value === null || lengthBytesUTF8(value) !== $2) return 0;
            stringToUTF8(value, $1, $2 + 1);
            return 1;
        } catch (error) { return 0; }
    }, path.c_str(), contents.data(), size);
    contents.resize(static_cast<std::size_t>(size));
    return copied != 0;
#else
    std::error_code fileError;
    fileExists = std::filesystem::exists(filePath, fileError);
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
    if (fileError || fileSize > maximumSize)
    {
        return false;
    }

    std::ifstream input(filePath, std::ios::binary);
    if (!input)
    {
        return false;
    }
    contents.resize(static_cast<std::size_t>(fileSize));
    input.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (input.gcount() != static_cast<std::streamsize>(contents.size()) || input.bad())
    {
        return false;
    }
    // Refuse a changing file rather than reading beyond the bounded snapshot.
    return input.peek() == std::char_traits<char>::eof() && !input.bad();
#endif
}

bool ensureSaveDirectory(const std::filesystem::path& filePath)
{
    std::error_code directoryError;
    std::filesystem::path parentPath = filePath.parent_path();
    if (!parentPath.empty()) { std::filesystem::create_directories(parentPath, directoryError); }
    return !directoryError;
}

bool replaceSaveContents(const std::filesystem::path& filePath, const std::string& contents)
{
#if defined(__EMSCRIPTEN__)
    std::string path = filePath.string();
    return EM_ASM_INT({
        try {
            localStorage.setItem("wolcan:" + UTF8ToString($0), UTF8ToString($1));
            return 1;
        } catch (error) { return 0; }
    }, path.c_str(), contents.c_str()) != 0;
#else
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
#endif
}

namespace
{
bool loadRecords(const std::filesystem::path& filePath, std::map<std::string, double>& records)
{
    std::string contents;
    bool fileExists = false;
    if (!readSaveContents(filePath, 1024 * 1024, contents, fileExists)) { return false; }
    if (!fileExists) { return true; }

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
        if (!(row >> levelKey >> seconds) || (row >> extraToken) || !isValidSaveKey(levelKey) || !std::isfinite(seconds) || seconds < 0.0)
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
}

double readBestTime(const std::string& filePath, const std::string& levelKey)
{
    if (!isValidSaveKey(levelKey) || !isValidSavePath(filePath))
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
    if (!isValidSaveKey(levelKey) || !isValidSavePath(filePath) || !std::isfinite(seconds) || seconds < 0.0)
    {
        return false;
    }
    try
    {
        std::filesystem::path targetPath(filePath);
        return withSaveLock(targetPath, [&]
        {
            std::map<std::string, double> records;
            if (!loadRecords(targetPath, records)) { return false; }
            auto recordPosition = records.find(levelKey);
            if (recordPosition != records.end() && recordPosition->second <= seconds) { return true; }
            if (recordPosition == records.end() && records.size() >= 4096) { return false; }

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
            return output && replaceSaveContents(targetPath, output.str());
        });
    }
    catch (...)
    {
        return false;
    }
}
