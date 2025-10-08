#include "ConfigManager.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool ConfigManager::loadConfig(DatabaseConfig& config) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cout << "Config file not found, creating default config..." << std::endl;
        
        config.language = "en"; 
        config.host = "localhost";
        config.port = 5432;
        config.database = "student_db";
        config.username = "postgres";
        config.password = "password";
        
        saveConfig(config);
        currentConfig = config;
        return true;
    }
    
    try {
        json j;
        file >> j;
        
        config.language = j.value("language", "en");
        config.host = j.value("host", "localhost");
        config.port = j.value("port", 5432);
        config.database = j.value("database", "student_db");
        config.username = j.value("username", "postgres");
        config.password = j.value("password", "password");
        
        currentConfig = config;
        //std::cout << "Config loaded successfully from " << configFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::saveConfig(const DatabaseConfig& config) {
    try {
        json j;
        
        j["language"] = config.language;
        j["host"] = config.host;
        j["port"] = config.port;
        j["database"] = config.database;
        j["username"] = config.username;
        j["password"] = config.password;
        
        std::ofstream file(configFile);
        file << j.dump(4);
        
        currentConfig = config; // Обновляем текущую конфигурацию
        std::cout << "Config saved to " << configFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::configExists() {
    std::ifstream file(configFile);
    return file.good();
}