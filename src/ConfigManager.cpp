#include "ConfigManager.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// Загружает конфигурацию из JSON файла
bool ConfigManager::loadConfig(DatabaseConfig& config) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cout << "Config file not found, creating default config..." << std::endl;
        
        // Создаем конфигурацию по умолчанию
        config.host = "localhost";
        config.port = 5432;
        config.database = "student_db";
        config.username = "postgres";
        config.password = "password";
        
        // Сохраняем конфигурацию по умолчанию
        saveConfig(config);
        return true;
    }
    
    try {
        json j;
        file >> j;  // Читаем JSON из файла
        
        // Берем значения из JSON
        config.host = j.value("host", "localhost");
        config.port = j.value("port", 5432);
        config.database = j.value("database", "student_db");
        config.username = j.value("username", "postgres");
        config.password = j.value("password", "password");
        
        std::cout << "Config loaded successfully from " << configFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

// Сохраняет конфигурацию в JSON файл
bool ConfigManager::saveConfig(const DatabaseConfig& config) {
    try {
        json j;  // Создаем JSON объект
        
        // Заполняем значениями из конфигурации
        j["host"] = config.host;
        j["port"] = config.port;
        j["database"] = config.database;
        j["username"] = config.username;
        j["password"] = config.password;
        
        // Сохраняем в файл с красивым форматированием (отступ 4 пробела)
        std::ofstream file(configFile);
        file << j.dump(4);
        
        std::cout << "Config saved to " << configFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

// Проверяет существует ли файл конфигурации
bool ConfigManager::configExists() {
    std::ifstream file(configFile);
    return file.good();  // Возвращает true если файл существует и доступен для чтения
}