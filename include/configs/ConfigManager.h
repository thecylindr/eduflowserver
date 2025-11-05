#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "models/Models.h"

class ConfigManager {
public:
    bool loadConfig(DatabaseConfig& config);
    bool saveConfig(const DatabaseConfig& config);
    bool dbConfigExists();
    bool loadApiConfig(ApiConfig& config);
    bool saveApiConfig(const ApiConfig& config);
    bool apiConfigExists();
    DatabaseConfig getDefaultDbConfig();
    ApiConfig getDefaultApiConfig();

private:
    DatabaseConfig currentDbConfig;
    ApiConfig currentApiConfig;
    const std::string dbConfigFile = "database_config.json";
    const std::string apiConfigFile = "api_config.json";
};

#endif