/*
** CrabeLoader
** File description:
** exception
*/

#include "loader/exception.hpp"
#include "logger/logger.hpp"

#include <cstdlib>

HookException::HookException(Severity severity, std::string message)
    : _severity(severity), _message(std::move(message))
{
}

const char* HookException::what() const noexcept
{
    return _message.c_str();
}

Severity HookException::severity() const noexcept
{
    return _severity;
}

void HookException::log() const
{
    Logger& logger = Logger::getInstance();

    switch (_severity) {
        case Severity::LOG:
            logger.info("{}", _message);
            break;
        case Severity::WARNING:
            logger.warning("{}", _message);
            break;
        case Severity::ERR:
            logger.error("{}", _message);
            break;
        case Severity::CRASH:
            logger.error("CRASH: {}", _message);
            std::abort();
    }
}
