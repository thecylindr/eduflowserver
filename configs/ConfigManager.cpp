#include "configs/ConfigManager.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool ConfigManager::loadConfig(DatabaseConfig& config) {
    std::ifstream file(dbConfigFile);
    if (!file.is_open()) {
        //std::cout << "Database config file not found, creating default config..." << std::endl;
        
        config = getDefaultDbConfig();
        saveConfig(config);
        currentDbConfig = config;
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
        
        currentDbConfig = config;
        //std::cout << "Database config loaded successfully from " << dbConfigFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading database config: " << e.what() << std::endl;
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
        
        std::ofstream file(dbConfigFile);
        file << j.dump(4);
        
        currentDbConfig = config;
        //std::cout << "Database config saved to " << dbConfigFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving database config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::dbConfigExists() {
    std::ifstream file(dbConfigFile);
    return file.good();
}

bool ConfigManager::loadApiConfig(ApiConfig& config) {
    std::ifstream file(apiConfigFile);
    if (!file.is_open()) {
        std::cout << "API config file not found, creating default config..." << std::endl;
        
        config = getDefaultApiConfig();
        saveApiConfig(config);
        currentApiConfig = config;
        return true;
    }
    
    try {
        json j;
        file >> j;
        
        config.port = j.value("port", 5000);
        config.host = j.value("host", "0.0.0.0");
        config.maxConnections = j.value("maxConnections", 10);
        config.sessionTimeoutHours = j.value("sessionTimeoutHours", 24);
        config.resetTokenTimeoutMinutes = j.value("resetTokenTimeoutMinutes", 60);
        config.enableCors = j.value("enableCors", true);
        config.corsOrigin = j.value("corsOrigin", "*");
        config.enableSSL = j.value("enableSSL", false);
        config.sslCertPath = j.value("sslCertPath", "");
        config.sslKeyPath = j.value("sslKeyPath", "");
        config.rateLimitRequests = j.value("rateLimitRequests", 100);
        config.rateLimitWindow = j.value("rateLimitWindow", 60);
        
        currentApiConfig = config;
        //std::cout << "API config loaded successfully from " << apiConfigFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading API config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::saveApiConfig(const ApiConfig& config) {
    try {
        json j;
        
        j["port"] = config.port;
        j["host"] = config.host;
        j["maxConnections"] = config.maxConnections;
        j["sessionTimeoutHours"] = config.sessionTimeoutHours;
        j["resetTokenTimeoutMinutes"] = config.resetTokenTimeoutMinutes;
        j["enableCors"] = config.enableCors;
        j["corsOrigin"] = config.corsOrigin;
        j["enableSSL"] = config.enableSSL;
        j["sslCertPath"] = config.sslCertPath;
        j["sslKeyPath"] = config.sslKeyPath;
        j["rateLimitRequests"] = config.rateLimitRequests;
        j["rateLimitWindow"] = config.rateLimitWindow;
        
        std::ofstream file(apiConfigFile);
        file << j.dump(4);
        
        currentApiConfig = config;
        //std::cout << "API config saved to " << apiConfigFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving API config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::apiConfigExists() {
    std::ifstream file(apiConfigFile);
    return file.good();
}

DatabaseConfig ConfigManager::getDefaultDbConfig() {
    DatabaseConfig config;
    config.language = "en";
    config.host = "localhost";
    config.port = 5432;
    config.database = "student_db";
    config.username = "postgres";
    config.password = "password";
    return config;
}

ApiConfig ConfigManager::getDefaultApiConfig() {
    ApiConfig config;
    config.port = 5000;
    config.host = "0.0.0.0";
    config.maxConnections = 10;
    config.sessionTimeoutHours = 24;
    config.resetTokenTimeoutMinutes = 60;
    config.enableCors = true;
    config.corsOrigin = "*";
    config.enableSSL = false;
    config.sslCertPath = "";
    config.sslKeyPath = "";
    config.rateLimitRequests = 100;
    config.rateLimitWindow = 60;
    return config;
}