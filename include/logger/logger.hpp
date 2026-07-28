/*
** EPITECH PROJECT, 2026
** CrabeLoader
** File description:
** logger
*/

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <format>
#include <utility>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERR
};

class Logger {
public:
    static Logger& getInstance();

    void setLogFile(const std::string& filename);
    void setLogLevel(LogLevel level);

    template <typename... Args>
    void log(LogLevel level, std::string_view fmt, Args&&... args) {
        if (level < m_minLevel) {
            return;
        }
        std::string message = std::vformat(fmt, std::make_format_args(args...));
        writeLog(level, message);
    }

    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args) {
        log(LogLevel::DEBUG, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args) {
        log(LogLevel::INFO, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(std::string_view fmt, Args&&... args) {
        log(LogLevel::WARNING, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::string_view fmt, Args&&... args) {
        log(LogLevel::ERR, fmt, std::forward<Args>(args)...);
    }

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeLog(LogLevel level, std::string_view message);

    std::string levelToString(LogLevel level) const;
    std::string getCurrentTime() const;

    std::ofstream m_fileStream;
    LogLevel m_minLevel;
    std::mutex m_mutex;
};

#endif // LOGGER_HPP
