/*
** CrabeLoader
** File description:
** logger
*/

#include "logger/logger.hpp"

#include <chrono>
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
        m_fileStream.flush();
    }

    m_history.push_back(std::move(formattedMessage));
    if (m_history.size() > kMaxHistoryLines) {
        m_history.pop_front();
    }
}

std::vector<std::string> Logger::getHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<std::string>(m_history.begin(), m_history.end());
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

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S")
    << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}