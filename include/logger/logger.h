#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

class Logger {
private:
    std::ofstream logFile;
    std::mutex logMutex;
    std::string logFileName;
    static Logger* instance;
    
    Logger();
    ~Logger();
    void rotateIfNeeded();

public:
    static Logger& getInstance();
    void log(const std::string& message, const std::string& level = "INFO");
    std::vector<std::string> getLastLines(int lineCount = 40);
    void clearLogs();
    std::string getLogFilePath() const { return logFileName; }
};

#endif