// main.cpp - версия с цветным оформлением
#include <iostream>
#include <string>
#include <cstdlib>
#include <limits>
#include <iomanip>
#include <thread>
#include "database/DatabaseService.h"
#include "api/ApiService.h"
#include "configs/ConfigManager.h"
#include "LocaleManager.h"

#include <vector>
#include <map>
#include <chrono>
#include <sstream>
#include "models/Models.h"

#ifdef _WIN32
#include <windows.h>
#define CLEAR_SCREEN "cls"
#else
#define CLEAR_SCREEN "clear"
#endif

// Цветовые коды для консоли
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string BOLD = "\033[1m";
}

class Application {
private:
    DatabaseService dbService;
    ApiService apiService;
    ConfigManager configManager;
    bool apiRunning = false;
    std::map<std::string, std::string> locale;

public:
    Application(const std::map<std::string, std::string>& loc) : apiService(dbService), locale(loc) {}

    // Вспомогательные методы для локализации
    std::string tr(const std::string& key) {
        auto it = locale.find(key);
        if (it != locale.end()) {
            return it->second;
        }
        return "[" + key + "]";
    }

    // Очистка экрана
    void clearScreen() {
        system(CLEAR_SCREEN);
    }

    // Красивый заголовок с цветом
    void drawHeader(const std::string& title) {
        std::cout << Colors::MAGENTA << "┌────────────────────────────────────────────────────────────┐" << Colors::RESET << std::endl;
        std::cout << Colors::MAGENTA << "                🎓 " << title << " 🎓                  " << Colors::RESET << std::endl;
        std::cout << Colors::MAGENTA << "└────────────────────────────────────────────────────────────┘" << Colors::RESET << std::endl;
    }

    // Информационное сообщение
    void showInfo(const std::string& message) {
        std::cout << Colors::CYAN << "💡 " << message << Colors::RESET << std::endl;
    }

    // Сообщение об успехе
    void showSuccess(const std::string& message) {
        std::cout << Colors::GREEN << "✅ " << message << Colors::RESET << std::endl;
    }

    // Сообщение об ошибке
    void showError(const std::string& message) {
        std::cout << Colors::RED << "❌ " << message << Colors::RESET << std::endl;
    }

    // Предупреждение
    void showWarning(const std::string& message) {
        std::cout << Colors::YELLOW << "⚠️  " << message << Colors::RESET << std::endl;
    }

    // Смена языка
    void changeLanguage() {
        clearScreen();
        drawHeader(tr("language_selection"));
        
        std::cout << Colors::MAGENTA << "🌍 " << tr("select_language") << ":" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "  1. English" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "  2. Русский" << Colors::RESET << std::endl;
        std::cout << std::endl << Colors::YELLOW << "🎯 " << tr("choose_option") << ": " << Colors::RESET;
        
        std::string choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        std::string newLanguage;
        if (choice == "1") {
            newLanguage = "en";
        } else if (choice == "2") {
            newLanguage = "ru";
        } else {
            showError(tr("invalid_choice"));
            waitForEnter();
            return;
        }
        
        auto newLocale = LocaleManager::loadLocale(newLanguage);
        if (newLocale.empty()) {
            showError(tr("language_load_failed"));
        } else {
            locale = newLocale;
            
            DatabaseConfig config = dbService.getCurrentConfig();
            config.language = newLanguage;
            configManager.saveConfig(config);
            
            showSuccess(tr("language_changed"));
        }

        waitForEnter();
    }

    // Отображает главное меню
    void showMainMenu() {
        while (true) {
            clearScreen();
            drawHeader(tr("app_title"));
            
            // Статус системы
            std::cout << Colors::MAGENTA << "📊 " << tr("system_status") << ":" << Colors::RESET << std::endl;
            std::cout << "   🗄️  " << tr("database") << ": " 
                      << (dbService.testConnection() ? Colors::GREEN + "✅ " + tr("connected") : Colors::RED + "❌ " + tr("disconnected")) 
                      << Colors::RESET << std::endl;
            std::cout << "   🌐 " << tr("api_server") << ": " 
                      << (apiRunning ? Colors::GREEN + "✅ " + tr("running") : Colors::RED + "❌ " + tr("stopped")) 
                      << Colors::RESET << std::endl;
            
            DatabaseConfig config = dbService.getCurrentConfig();
            std::cout << "   🌍 " << tr("language") << ": " 
                      << (config.language == "en" ? Colors::CYAN + "English" : Colors::CYAN + "Русский") 
                      << Colors::RESET << std::endl;
            
            std::cout << std::endl;
            std::cout << Colors::MAGENTA << "📋 " << tr("main_menu") << ":" << Colors::RESET << std::endl;
            std::cout << std::endl;
            
            // Цветное меню
            std::cout << Colors::CYAN << "1. ⚙️  " << tr("menu_db_setup") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "2. 🌐 " << tr("menu_api_setup") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "3. 🚀 " << tr("menu_api_manage") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "4. ℹ️   " << tr("menu_system_info") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "5. 🌍 " << tr("menu_change_language") << Colors::RESET << std::endl;
            std::cout << Colors::RED << "Q. 🚪 " << tr("menu_exit") << Colors::RESET << std::endl;
            
            std::cout << std::endl << Colors::YELLOW << "🎯 " << tr("choose_option") << ": " << Colors::RESET;
            std::string choice;
            std::cin >> choice;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            
            if (choice == "1") {
                setupDatabase();
            } else if (choice == "2") {
                setupApiConfig();
            } else if (choice == "3") {
                manageApi();
            } else if (choice == "4") {
                showSystemInfo();
            } else if (choice == "5") {
                changeLanguage();
            } else if (choice == "Q" || choice == "q") {
                exitApplication();
                break;
            } else {
                showError(tr("invalid_choice"));
                waitForEnter();
            }
        }
    }

private:
    // Настройка базы данных
    void setupDatabase() {
        clearScreen();
        drawHeader(tr("db_config_title"));

        DatabaseConfig currentConfig = dbService.getCurrentConfig();
        
        std::cout << Colors::MAGENTA << "📄 " << tr("current_settings") << ":" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   📍 " << tr("host") << ": " << Colors::WHITE << currentConfig.host << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   🚪 " << tr("port") << ": " << Colors::WHITE << currentConfig.port << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   🗄️  " << tr("database_name") << ": " << Colors::WHITE << currentConfig.database << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   👤 " << tr("username") << ": " << Colors::WHITE << currentConfig.username << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   🔒 " << tr("password") << ": " << Colors::WHITE << std::string(currentConfig.password.length(), '*') << Colors::RESET << std::endl;
        
        std::cout << std::endl << Colors::YELLOW << "🔄 " << tr("change_settings") << " " << Colors::RESET;
        std::string change;
        std::getline(std::cin, change);
        
        if (change == "y" || change == "Y" || change == "да" || change == "д") {
            std::cout << std::endl << Colors::MAGENTA << "✏️  " << tr("enter_new_settings") << ":" << Colors::RESET << std::endl;
            
            std::cout << Colors::CYAN << "   " << tr("host") << " [" << currentConfig.host << "]: " << Colors::RESET;
            std::string host;
            std::getline(std::cin, host);
            if (!host.empty()) currentConfig.host = host;

            std::cout << Colors::CYAN << "   " << tr("port") << " [" << currentConfig.port << "]: " << Colors::RESET;
            std::string portStr;
            std::getline(std::cin, portStr);
            if (!portStr.empty()) currentConfig.port = std::stoi(portStr);

            std::cout << Colors::CYAN << "   " << tr("database_name") << " [" << currentConfig.database << "]: " << Colors::RESET;
            std::string db;
            std::getline(std::cin, db);
            if (!db.empty()) currentConfig.database = db;

            std::cout << Colors::CYAN << "   " << tr("username") << " [" << currentConfig.username << "]: " << Colors::RESET;
            std::string user;
            std::getline(std::cin, user);
            if (!user.empty()) currentConfig.username = user;

            std::cout << Colors::CYAN << "   " << tr("password") << ": " << Colors::RESET;
            std::string pass;
            std::getline(std::cin, pass);
            if (!pass.empty()) currentConfig.password = pass;

            if (configManager.saveConfig(currentConfig)) {
                showSuccess(tr("settings_saved"));
            } else {
                showError("Failed to save settings");
            }
        }

        std::cout << std::endl << Colors::YELLOW << "🔍 " << tr("testing_connection") << "..." << Colors::RESET << std::endl;
        if (dbService.testConnection()) {
            showSuccess(tr("connection_success"));
            std::cout << Colors::YELLOW << "⚙️  " << tr("setting_up_tables") << "..." << Colors::RESET << std::endl;
            if (dbService.setupDatabase()) {
                showSuccess(tr("db_setup_success"));
            } else {
                showError(tr("db_setup_error"));
            }
        } else {
            showError(tr("connection_error"));
            showInfo(tr("check_settings"));
        }

        waitForEnter();
    }

    // Настройка конфигурации API
    void setupApiConfig() {
        clearScreen();
        drawHeader(tr("api_setup_title"));
        
        // Загружаем текущую конфигурацию для отображения
        ApiConfig currentConfig;
        configManager.loadApiConfig(currentConfig);
        
        std::cout << Colors::MAGENTA << "📄 " << tr("current_api_settings") << ":" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   📍 " << tr("host") << ": " << Colors::WHITE << currentConfig.host << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   🚪 " << tr("port") << ": " << Colors::WHITE << currentConfig.port << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   🔄 " << "Max Connections: " << Colors::WHITE << currentConfig.maxConnections << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   ⏱️  " << "Session Timeout: " << Colors::WHITE << currentConfig.sessionTimeoutHours << " hours" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   🔒 " << "Enable CORS: " << Colors::WHITE << (currentConfig.enableCors ? "Yes" : "No") << Colors::RESET << std::endl;
        
        std::cout << std::endl << Colors::YELLOW << "🔄 " << tr("change_api_settings") << " " << Colors::RESET;
        std::string change;
        std::getline(std::cin, change);
        
        if (change == "y" || change == "Y" || change == "да" || change == "д") {
            std::cout << std::endl << Colors::MAGENTA << "✏️  " << tr("enter_new_api_settings") << ":" << Colors::RESET << std::endl;
            
            std::cout << Colors::CYAN << "   " << tr("host") << " [" << currentConfig.host << "]: " << Colors::RESET;
            std::string host;
            std::getline(std::cin, host);
            if (!host.empty()) currentConfig.host = host;

            std::cout << Colors::CYAN << "   " << tr("port") << " [" << currentConfig.port << "]: " << Colors::RESET;
            std::string portStr;
            std::getline(std::cin, portStr);
            if (!portStr.empty()) currentConfig.port = std::stoi(portStr);

            std::cout << Colors::CYAN << "   " << "Max Connections [" << currentConfig.maxConnections << "]: " << Colors::RESET;
            std::string maxConnStr;
            std::getline(std::cin, maxConnStr);
            if (!maxConnStr.empty()) currentConfig.maxConnections = std::stoi(maxConnStr);

            std::cout << Colors::CYAN << "   " << "Session Timeout (hours) [" << currentConfig.sessionTimeoutHours << "]: " << Colors::RESET;
            std::string timeoutStr;
            std::getline(std::cin, timeoutStr);
            if (!timeoutStr.empty()) currentConfig.sessionTimeoutHours = std::stoi(timeoutStr);

            std::cout << Colors::CYAN << "   " << "Enable CORS (y/n) [" << (currentConfig.enableCors ? "y" : "n") << "]: " << Colors::RESET;
            std::string corsStr;
            std::getline(std::cin, corsStr);
            if (!corsStr.empty()) {
                currentConfig.enableCors = (corsStr == "y" || corsStr == "Y" || corsStr == "да");
            }

            if (configManager.saveApiConfig(currentConfig)) {
                showSuccess(tr("api_settings_saved"));
                
                if (apiRunning) {
                    showWarning(tr("api_restart_required"));
                }
            } else {
                showError("Failed to save API settings");
            }
        }

        waitForEnter();
    }

    // Управление API сервером
    void manageApi() {
        clearScreen();
        drawHeader(tr("api_manage_title"));
        
        if (apiRunning) {
            showSuccess(tr("api_already_running"));
            
            // Получаем текущую конфигурацию для отображения правильного адреса
            ApiConfig currentConfig;
            configManager.loadApiConfig(currentConfig);
            
            std::cout << Colors::CYAN << "📍 " << tr("available_at") << ": " << Colors::WHITE 
                      << "http://" << currentConfig.host << ":" << currentConfig.port << Colors::RESET << std::endl;
            std::cout << std::endl;
            
            std::cout << Colors::YELLOW << "🛑 " << tr("stop_api_prompt") << " " << Colors::RESET;
            
            std::string choice;
            std::getline(std::cin, choice);
            
            if (choice == "y" || choice == "Y" || choice == "да" || choice == "д") {
                std::cout << Colors::YELLOW << "⏳ Останавливаем API сервер..." << Colors::RESET << std::endl;
                
                apiService.stop();
                apiRunning = false;
                
                // Даем больше времени для корректной остановки
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                
                showSuccess(tr("api_stop_success"));
            } else {
                showInfo(tr("api_keep_running"));
            }
        } else {
            std::cout << Colors::YELLOW << "🔍 " << tr("checking_db") << "..." << Colors::RESET << std::endl;
            if (dbService.testConnection()) {
                showSuccess(tr("db_available"));
                std::cout << Colors::YELLOW << "🚀 " << tr("starting_api") << "..." << Colors::RESET << std::endl;
                
                // API Service сам загружает конфигурацию при запуске
                if (apiService.start()) {
                    apiRunning = true;
                    std::cout << std::endl;
                    showSuccess(tr("api_start_success"));
                    
                    // Получаем текущую конфигурацию для отображения правильного адреса
                    ApiConfig currentConfig;
                    configManager.loadApiConfig(currentConfig);
                    
                    std::cout << Colors::CYAN << "📍 " << tr("available_at") << ": " << Colors::WHITE 
                              << "http://" << currentConfig.host << ":" << currentConfig.port << Colors::RESET << std::endl;
                    std::cout << std::endl << Colors::MAGENTA << "📡 " << tr("available_endpoints") << ":" << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   👥 GET /students   - " << Colors::WHITE << "Students management" << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   👨‍🏫 GET /teachers  - " << Colors::WHITE << "Teachers management" << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   🎯 GET /groups     - " << Colors::WHITE << "Groups management" << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   📁 GET /portfolio  - " << Colors::WHITE << "Portfolio management" << Colors::RESET << std::endl;
                } else {
                    showError(tr("api_start_error"));
                }
            } else {
                showError(tr("db_unavailable"));
                showInfo(tr("setup_db_first"));
            }
        }

        waitForEnter();
    }

    // Информация о системе
    void showSystemInfo() {
        clearScreen();
        drawHeader(tr("system_info_title"));
        
        std::cout << Colors::CYAN << "🎯 " << tr("app_title") << ": " << Colors::WHITE << "1.0" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "🖥️  Platform: " << Colors::WHITE << "Windows/Linux" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "🗄️  Database: " << Colors::WHITE << "PostgreSQL 12+" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "💻 Programming Language: " << Colors::WHITE << "C++17" << Colors::RESET << std::endl;
        
        std::cout << std::endl << Colors::MAGENTA << "📚 Used Libraries:" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • libpq " << Colors::WHITE << "(PostgreSQL client)" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • nlohmann/json " << Colors::WHITE << "(JSON processing)" << Colors::RESET << std::endl;
        
        std::cout << std::endl << Colors::MAGENTA << "🚀 Main Features:" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • " << Colors::WHITE << "Database Management" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • " << Colors::WHITE << "REST API Server" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • " << Colors::WHITE << "Multi-language Support" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • " << Colors::WHITE << "Cross-platform" << Colors::RESET << std::endl;
        
        std::cout << std::endl << Colors::MAGENTA << "👨‍💻 Developer: " << Colors::WHITE << "Dmitry Stolbov" << Colors::RESET << std::endl;

        waitForEnter();
    }

    // Выход из приложения
    void exitApplication() {
        clearScreen();
        drawHeader(tr("exit_title"));
        
        if (apiRunning) {
            std::cout << Colors::YELLOW << "🛑 " << tr("stopping_api") << "..." << Colors::RESET << std::endl;
            apiService.stop();
            apiRunning = false;
            // Ждем больше времени для корректной остановки
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            showSuccess(tr("api_stop_success"));
        }
        
        std::cout << std::endl << Colors::GREEN << "👋 " << tr("thank_you") << Colors::RESET << std::endl;
        std::cout << std::endl;
    }

    // Вспомогательные методы
    void waitForEnter() {
        std::cout << std::endl << Colors::YELLOW << "↵ " << tr("press_enter") << Colors::RESET << std::endl;
        std::cin.get();
    }
};

int main() {
    // Настройка кодировки для Windows
#ifdef _WIN32
    SetConsoleOutputCP(65001); // UTF-8
    SetConsoleCP(65001);
#endif

    try {
        // Проверка файлов локализации
        std::cout << Colors::YELLOW << "🌍 Checking localization files..." << Colors::RESET << std::endl;
        if (!LocaleManager::checkLocales()) {
            std::cout << Colors::RED << "❌ Localization files missing. Please make sure lang/locale_en.json and lang/locale_ru.json exist." << Colors::RESET << std::endl;
            return 1;
        }

        // Загружаем конфигурацию для определения языка
        ConfigManager configManager;
        DatabaseConfig config;
        if (!configManager.loadConfig(config)) {
            std::cerr << Colors::RED << "❌ Failed to load configuration" << Colors::RESET << std::endl;
            return 1;
        }
        
        // Загружаем локализацию на основе конфигурации
        std::map<std::string, std::string> currentLocale = LocaleManager::loadLocale(config.language);
        
        if (currentLocale.empty()) {
            std::cerr << Colors::RED << "❌ Failed to load localization for language: " << config.language << Colors::RESET << std::endl;
            std::cerr << Colors::YELLOW << "⚠️  Trying to load English localization..." << Colors::RESET << std::endl;
            
            currentLocale = LocaleManager::loadLocale("en");
            if (currentLocale.empty()) {
                std::cerr << Colors::RED << "❌ Failed to load English localization. Please check your localization files." << Colors::RESET << std::endl;
                return 1;
            }
        }

        std::cout << std::endl << Colors::GREEN << "🚀 Starting application..." << Colors::RESET << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        Application app(currentLocale);
        app.showMainMenu();
    } catch (const std::exception& e) {
        std::cerr << Colors::RED << "💥 Critical error: " << e.what() << Colors::RESET << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    return 0;
}