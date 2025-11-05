// LocaleManager.h
#ifndef LOCALEMANAGER_H
#define LOCALEMANAGER_H

#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include "json.hpp"

using json = nlohmann::json;

class LocaleManager {
public:
    static bool checkLocales();
    static std::map<std::string, std::string> loadLocale(const std::string& language);
    static void showLanguageSelection(std::map<std::string, std::string>& currentLocale);
};

#endif