/**
 * @file logger.h
 * @brief Pełny system logowania diagnostycznego z obsługą poziomów błędów.
 * @version 1.0.0
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

class Logger {
public:
    enum class LogType { Info, Error, Wrong };

    static Logger& getInstance();
    void log(const std::string& message, LogType type = LogType::Info);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile;
    std::ofstream wrongLogFile;
    bool wrongLoggingEnabled;
    std::mutex logMutex;
};

#endif // LOGGER_H

