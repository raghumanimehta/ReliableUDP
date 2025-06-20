#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

// Log levels
enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// Global state (file-static variables)
static LogLevel g_currentLogLevel = INFO;
static std::ofstream g_logFile;

// Core logging function
inline void log_message(LogLevel level, const std::string& message) {
    // Check if we should log this message
    if (level < g_currentLogLevel) {
        return;
    }

    // Get level string
    std::string levelStr;
    switch (level) {
        case DEBUG:   levelStr = "DEBUG"; break;
        case INFO:    levelStr = "INFO"; break;
        case WARNING: levelStr = "WARNING"; break;
        case ERROR:   levelStr = "ERROR"; break;
    }

    // Get current time
    time_t now = time(nullptr);
    char timeBuffer[80];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Format log entry
    std::string logEntry = std::string(timeBuffer) + " [" + levelStr + "] " + message;

    // Output to console
    std::cout << logEntry << std::endl;

    // Output to file if opened
    if (g_logFile.is_open()) {
        g_logFile << logEntry << std::endl;
        g_logFile.flush();
    }
}

// Configuration functions
inline void set_log_level(LogLevel level) {
    g_currentLogLevel = level;
}

inline void set_log_file(const std::string& filename) {
    if (g_logFile.is_open()) {
        g_logFile.close();
    }
    g_logFile.open(filename, std::ios::app);
}

inline void close_log_file() {
    if (g_logFile.is_open()) {
        g_logFile.close();
    }
}

// Convenience macros
#define LOG_DEBUG(msg)   log_message(DEBUG, msg)
#define LOG_INFO(msg)    log_message(INFO, msg)
#define LOG_WARNING(msg) log_message(WARNING, msg)
#define LOG_ERROR(msg)   log_message(ERROR, msg)

#endif // LOGGER_H