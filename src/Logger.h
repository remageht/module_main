#pragma once
// Minimal structured stdout logger: [ts][LEVEL][conn=N] message. Thread-safe.

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace gateway {

enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3 };

inline LogLevel& globalLogLevel() {
    static LogLevel lvl = LogLevel::Info;
    return lvl;
}

inline void setLogLevel(const std::string& name) {
    if (name == "debug") globalLogLevel() = LogLevel::Debug;
    else if (name == "warn" || name == "warning") globalLogLevel() = LogLevel::Warn;
    else if (name == "error") globalLogLevel() = LogLevel::Error;
    else globalLogLevel() = LogLevel::Info;
}

inline const char* levelName(LogLevel l) {
    switch (l) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "INFO";
    }
}

inline std::mutex& logMutex() {
    static std::mutex m;
    return m;
}

inline void gwLog(LogLevel lvl, long long connId, const std::string& msg) {
    if (lvl < globalLogLevel()) return;
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmv, "%Y-%m-%dT%H:%M:%S");
    std::lock_guard<std::mutex> lock(logMutex());
    if (connId >= 0)
        std::cout << "[" << oss.str() << "][" << levelName(lvl) << "][conn=" << connId << "] "
                  << msg << std::endl;
    else
        std::cout << "[" << oss.str() << "][" << levelName(lvl) << "] " << msg << std::endl;
}

}  // namespace gateway
