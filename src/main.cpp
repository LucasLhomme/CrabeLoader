/*
** CrabeLoader
** File description:
** main file for the CrabeLoader project
*/

#include "minhook/lib/proxy/proxy.hpp" // BINKW32 PROXY DIRECT LINKER FORWARDING
#include <windows.h>
#include "logger/logger.hpp"


const void init_logger() {
    Logger& logger = Logger::getInstance();
    logger.setLogFile("CrabeLoader.log");
    logger.setLogLevel(LogLevel::DEBUG);
    logger.info("Logger initialized.");
}

int main()
{
    init_logger();
    Logger::getInstance().info("CrabeLoader started.");
    return 0;
}