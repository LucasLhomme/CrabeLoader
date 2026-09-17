/*
** CrabeLoader
** File description:
** logger
*/

#include "shared/logger.hpp"

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : m_minLevel(LogLevel::INFO) {}

Logger::~Logger() {
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    m_fileStream.open(filename, std::ios::app);
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLevel = level;
}

void Logger::writeLog(LogLevel level, std::string_view message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string formattedMessage = std::format("[{}] [{}] {}\n", 
                                                getCurrentTime(), 
                                                levelToString(level), 
                                                message);

    if (level == LogLevel::ERR) {
        std::cerr << formattedMessage;
    } else {
        std::cout << formattedMessage;
    }

    if (m_fileStream.is_open()) {
        m_fileStream << formattedMessage;
        // A flush is a write syscall, and DEBUG alone emits ~1500 lines per
        // boot tracing loadbuffer chunks -- that traffic is what the stream's
        // buffer is for. Everything above DEBUG is rare and is what a crash
        // report is read for, so it still goes out immediately; the flush
        // carries any buffered DEBUG lines with it.
        if (level != LogLevel::DEBUG)
            m_fileStream.flush();
    }

    m_history.push_back(LogEntry{ level, std::move(formattedMessage) });
    if (m_history.size() > kMaxHistoryLines) {
        m_history.pop_front();
    }
}

std::vector<LogEntry> Logger::getHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<LogEntry>(m_history.begin(), m_history.end());
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERR:     return "ERROR";
        default:                return "UNKNOWN";
    }
}

std::string Logger::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm local{};
    localtime_s(&local, &timeT);

    char buffer[32];
    int len = std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                            local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                            local.tm_hour, local.tm_min, local.tm_sec,
                            static_cast<int>(ms.count()));
    return std::string(buffer, len > 0 ? static_cast<size_t>(len) : 0);
}