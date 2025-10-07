#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "Models.h"
#include <string>

class ConfigManager {
public:
    bool loadConfig(DatabaseConfig& config);
    bool saveConfig(const DatabaseConfig& config);
    bool configExists();
    std::string getCurrentLanguage() const { return currentConfig.language; }
    
private:
    std::string configFile = "config.json";
    DatabaseConfig currentConfig; // Добавлено хранение конфигурации
};

#endif