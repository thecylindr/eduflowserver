#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "models/Models.h"
#include <string>

class ConfigManager {
private:
    DatabaseConfig currentDbConfig;
    ApiConfig currentApiConfig;
    const std::string dbConfigFile = "config.json";
    const std::string apiConfigFile = "api_config.json";
    
public:
    // Database config methods
    bool loadConfig(DatabaseConfig& config);
    bool saveConfig(const DatabaseConfig& config);
    bool dbConfigExists();
    
    // API config methods
    bool loadApiConfig(ApiConfig& config);
    bool saveApiConfig(const ApiConfig& config);
    bool apiConfigExists();
    
    // Getters
    std::string getCurrentLanguage() const { return currentDbConfig.language; }
    DatabaseConfig getCurrentDbConfig() const { return currentDbConfig; }
    ApiConfig getCurrentApiConfig() const { return currentApiConfig; }
    
    // Default configs
    DatabaseConfig getDefaultDbConfig();
    ApiConfig getDefaultApiConfig();
};

#endif