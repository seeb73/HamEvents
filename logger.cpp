/**
 * @file logger.cpp
 * @brief Implementacja systemu logowania zgodna z ConfigManager 1.x.
 * @version 1.0.0
 */

#include "include/logger.h"
#include "include/config_manager.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

Logger::Logger() : wrongLoggingEnabled(false) {
    const ConfigManager& config = ConfigManager::getInstance();
    logFile.open(config.getLogFilePath(), std::ios::app);
    wrongLoggingEnabled = config.isWrongLogEnabled();
    if (wrongLoggingEnabled) {
        wrongLogFile.open(config.getWrongLogFilePath(), std::ios::app);
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
    if (wrongLogFile.is_open()) {
        wrongLogFile.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(const std::string& message, LogType type) {
    std::lock_guard<std::mutex> lock(logMutex);

    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ";

    switch (type) {
    case LogType::Info:
        oss << "[INFO] ";
        break;
    case LogType::Error:
        oss << "[ERROR] ";
        break;
    case LogType::Wrong:
        oss << "[WRONG] ";
        break;
    }

    oss << message;

    if (type == LogType::Wrong && wrongLoggingEnabled && wrongLogFile.is_open()) {
        wrongLogFile << oss.str() << std::endl;
    } else if (logFile.is_open()) {
        logFile << oss.str() << std::endl;
    } else {
        std::cerr << "Logger error: Unable to write log." << std::endl;
    }
}

