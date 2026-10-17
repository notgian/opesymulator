#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

// Returns the current local time as "MM/DD/YYYY HH:MM:SSAM" (e.g. 08/06/2024 09:15:22AM).
// Thread-safe: std::localtime uses a shared static buffer, so access is serialised.
inline std::string currentTimestamp() {
    static std::mutex timeMutex;
    std::lock_guard<std::mutex> lock(timeMutex);

    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tmBuf = *std::localtime(&t);

    std::ostringstream ss;
    ss << std::put_time(&tmBuf, "%m/%d/%Y %I:%M:%S%p");
    return ss.str();
}
