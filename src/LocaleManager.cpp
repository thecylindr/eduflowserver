// LocaleManager.cpp
#include "LocaleManager.h"
#include <limits>

bool LocaleManager::checkLocales() {
    bool enExists = std::ifstream("lang/locale_en.json").good();
    bool ruExists = std::ifstream("lang/locale_ru.json").good();
    
    if (enExists && ruExists) {
        std::cout << "✅ Localization files found" << std::endl;
        return true;
    }
    
    if (!enExists) {
        std::cerr << "❌ locale_en.json not found in lang folder" << std::endl;
    }
    if (!ruExists) {
        std::cerr << "❌ locale_ru.json not found in lang folder" << std::endl;
    }
    
    return false;
}

std::map<std::string, std::string> LocaleManager::loadLocale(const std::string& language) {
    std::map<std::string, std::string> locale;
    std::string filename = "lang/locale_" + language + ".json";
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "❌ Cannot open locale file: " << filename << std::endl;
        return locale;
    }
    
    try {
        json j;
        file >> j;
        for (auto& el : j.items()) {
            locale[el.key()] = el.value();
        }
        std::cout << "✅ Loaded " << language << " localization" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error parsing locale file: " << e.what() << std::endl;
    }
    
    return locale;
}

void LocaleManager::showLanguageSelection(std::map<std::string, std::string>& currentLocale) {
    int choice;
    while (true) {
        system("cls||clear");
        
        std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                  LANGUAGE SELECTION                      ║" << std::endl;
        std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
        std::cout << "║  1. English                                              ║" << std::endl;
        std::cout << "║  2. Русский                                              ║" << std::endl;
        std::cout << "║                                                          ║" << std::endl;
        std::cout << "║  Please choose language (1-2):                           ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
        std::cout << "👉 Your choice: ";
        
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        if (choice == 1) {
            currentLocale = loadLocale("en");
            std::cout << "✅ Language changed to English" << std::endl;
            break;
        } else if (choice == 2) {
            currentLocale = loadLocale("ru");
            std::cout << "✅ Язык изменен на русский" << std::endl;
            break;
        } else {
            std::cout << "❌ Invalid choice! Please try again." << std::endl;
            std::cout << "Press Enter to continue...";
            std::cin.get();
        }
    }
}