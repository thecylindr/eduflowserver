#include "logger/logger.h"
#include <iostream>
#include <algorithm>

Logger* Logger::instance = nullptr;

Logger::Logger() {
    // Создаем папку logs если она не существует
    std::filesystem::create_directory("logs");
    
    // Имя файла с текущей датой
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << "logs/app_" 
       << std::put_time(&tm, "%Y%m%d") 
       << ".log";
    
    logFileName = ss.str();
    logFile.open(logFileName, std::ios::app);
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

Logger& Logger::getInstance() {
    if (!instance) {
        instance = new Logger();
    }
    return *instance;
}

void Logger::rotateIfNeeded() {
    if (logFile.is_open()) {
        logFile.close();
    }
    
    // Проверяем размер файла
    if (std::filesystem::exists(logFileName)) {
        auto fileSize = std::filesystem::file_size(logFileName);
        if (fileSize > 1024 * 1024) { // 1 MB
            // Создаем архивное имя с временной меткой
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&time_t);
            
            std::stringstream archiveName;
            archiveName << "logs/app_archive_" 
                       << std::put_time(&tm, "%Y%m%d_%H%M%S") 
                       << ".log";
            
            // Переименовываем текущий файл
            std::filesystem::rename(logFileName, archiveName.str());
        }
    }
    
    // Открываем файл заново
    logFile.open(logFileName, std::ios::app);
}

void Logger::log(const std::string& message, const std::string& level) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Проверяем и ротируем если нужно
    rotateIfNeeded();
    
    // Получаем текущее время
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    
    // Форматируем время
    std::stringstream timestamp;
    timestamp << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S]");
    
    // Записываем в файл
    if (logFile.is_open()) {
        logFile << timestamp.str() << " [" << level << "] " << message << std::endl;
        logFile.flush();
    }
}

std::vector<std::string> Logger::getLastLines(int lineCount) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::vector<std::string> lines;
    
    if (!std::filesystem::exists(logFileName)) {
        return lines;
    }
    
    std::ifstream file(logFileName);
    if (!file.is_open()) {
        return lines;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    file.close();
    
    // Возвращаем последние lineCount строк
    if (lines.size() > static_cast<size_t>(lineCount)) {
        lines.erase(lines.begin(), lines.end() - lineCount);
    }
    
    return lines;
}

void Logger::clearLogs() {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (logFile.is_open()) {
        logFile.close();
    }
    
    // Удаляем текущий лог файл
    if (std::filesystem::exists(logFileName)) {
        std::filesystem::remove(logFileName);
    }
    
    // Открываем новый чистый файл
    logFile.open(logFileName, std::ios::app);
}