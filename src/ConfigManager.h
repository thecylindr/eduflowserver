#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "Models.h"
#include <string>

class ConfigManager {
private:
    DatabaseConfig currentConfig;
    const std::string configFile = "config.json";
    
public:
    bool loadConfig(DatabaseConfig& config);
    bool saveConfig(const DatabaseConfig& config);
    bool configExists();
    std::string getCurrentLanguage() const { return currentConfig.language; }
};

#endif