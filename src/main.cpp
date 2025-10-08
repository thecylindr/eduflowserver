// main.cpp - исправленная версия
#include <iostream>
#include <string>
#include <cstdlib>
#include <limits>
#include <iomanip>
#include <thread>
#include "DatabaseService.h"
#include "ApiService.h"
#include "ConfigManager.h"
#include "LocaleManager.h"

#ifdef _WIN32
#include <windows.h>
#define CLEAR_SCREEN "cls"
#else
#define CLEAR_SCREEN "clear"
#endif

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
        return key; // Возвращаем ключ, если перевод не найден
    }

    // Очистка экрана
    void clearScreen() {
        system(CLEAR_SCREEN);
    }

    // Рисует красивый заголовок
    void drawHeader(const std::string& title) {
        int width = 60;
        int padding = (width - title.length() - 4) / 2;
        
        std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║";
        for (int i = 0; i < padding; i++) std::cout << " ";
        std::cout << "🎓 " << title << " 🎓";
        for (int i = 0; i < width - 4 - padding - title.length(); i++) std::cout << " ";
        std::cout << "║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl << std::endl;
    }

    // Рисует разделитель
    void drawSeparator() {
        std::cout << "──────────────────────────────────────────────────────────────" << std::endl;
    }

    // Смена языка
    void changeLanguage() {
        clearScreen();
        drawHeader(tr("language_selection"));
        
        std::cout << "🌍 " << tr("select_language") << ":" << std::endl;
        std::cout << "  1. English" << std::endl;
        std::cout << "  2. Русский" << std::endl;
        std::cout << std::endl << "🎯 " << tr("choose_option") << ": ";
        
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
        
        // Загружаем новую локализацию
        locale = LocaleManager::loadLocale(newLanguage);
        
        // Обновляем конфигурацию
        DatabaseConfig config = dbService.getCurrentConfig();
        config.language = newLanguage;
        configManager.saveConfig(config);
        
        showSuccess(tr("language_changed"));
        waitForEnter();
    }

    // Форматирование строки меню для выравнивания
    std::string formatMenuLine(const std::string& text, int width) {
        if (text.length() >= width) {
            return text.substr(0, width - 3) + "...";
        } else {
            return text + std::string(width - text.length(), ' ');
        }
    }

    // Отображает главное меню
    void showMainMenu() {
        while (true) {
            clearScreen();
            drawHeader(tr("app_title"));
            
            // Статус системы
            std::cout << "📊 " << tr("system_status") << ":" << std::endl;
            std::cout << "🗄️  " << tr("database") << ": " << (dbService.testConnection() ? "✅ " + tr("connected") : "❌ " + tr("disconnected")) << std::endl;
            std::cout << "🌐 " << tr("api_server") << ": " << (apiRunning ? "✅ " + tr("running") : "❌ " + tr("stopped")) << std::endl;
            
            // Получаем текущий язык из конфигурации
            DatabaseConfig config = dbService.getCurrentConfig();
            std::cout << "🌍 " << tr("language") << ": " << (config.language == "en" ? "English" : "Русский") << std::endl;
            
            std::cout << std::endl;
            
            drawSeparator();
            
            std::cout << "📋 " << tr("main_menu") << ":" << std::endl;
            std::cout << std::endl;
            
            // Подготовка строк меню
            std::string menu1 = "1. ⚙️  " + tr("menu_db_setup");
            std::string menu2 = "2. 🌐 " + tr("menu_api_manage");
            std::string menu3 = "3. 👥 " + tr("menu_students");
            std::string menu4 = "4. 👨‍🏫 " + tr("menu_teachers");
            std::string menu5 = "5. 🎯 " + tr("menu_groups");
            std::string menu6 = "6. 📁 " + tr("menu_portfolios");
            std::string menu7 = "7. ℹ️  " + tr("menu_system_info");
            std::string menu8 = "8. 🌍 " + tr("menu_change_language");
            std::string menuExit = "Q. 🚪 " + tr("menu_exit");
            
            std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
            std::cout << "║  " << formatMenuLine(menu1, 58) << "║" << std::endl;
            std::cout << "║  " << formatMenuLine(menu2, 58) << "║" << std::endl;
            std::cout << "║  " << formatMenuLine(menu3, 58) << "║" << std::endl;
            std::cout << "║  " << formatMenuLine(menu4, 58) << "║" << std::endl;
            std::cout << "║  " << formatMenuLine(menu5, 58) << "║" << std::endl;
            std::cout << "║  " << formatMenuLine(menu6, 58) << "║" << std::endl;
            std::cout << "║  " << formatMenuLine(menu7, 58) << "║" << std::endl;
            std::cout << "║  " << formatMenuLine(menu8, 58) << "║" << std::endl;
            std::cout << "║  " << std::string(58, ' ') << "║" << std::endl;
            std::cout << "║  " << formatMenuLine(menuExit, 58) << "║" << std::endl;
            std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
            
            std::cout << std::endl << "🎯 " << tr("choose_option") << ": ";
            std::string choice;
            std::cin >> choice;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            
            if (choice == "1") {
                setupDatabase();
            } else if (choice == "2") {
                manageApi();
            } else if (choice == "3") {
                manageStudents();
            } else if (choice == "4") {
                manageTeachers();
            } else if (choice == "5") {
                manageGroups();
            } else if (choice == "6") {
                managePortfolios();
            } else if (choice == "7") {
                showSystemInfo();
            } else if (choice == "8") {
                changeLanguage();
            } else if (choice == "Q" || choice == "q") {
                exitApplication();
                break;
            } else {
                showError(tr("invalid_choice"));
            }
        }
    }

private:
    // Настройка базы данных
    void setupDatabase() {
        clearScreen();
        drawHeader(tr("db_config_title"));

        DatabaseConfig currentConfig = dbService.getCurrentConfig();
        
        std::cout << "📄 " << tr("current_settings") << ":" << std::endl;
        std::cout << "   📍 " << tr("host") << ": " << currentConfig.host << std::endl;
        std::cout << "   🚪 " << tr("port") << ": " << currentConfig.port << std::endl;
        std::cout << "   🗄️ " << tr("database_name") << ": " << currentConfig.database << std::endl;
        std::cout << "   👤 " << tr("username") << ": " << currentConfig.username << std::endl;
        std::cout << "   🔒 " << tr("password") << ": " << std::string(currentConfig.password.length(), '*') << std::endl;
        
        std::cout << std::endl << "🔄 " << tr("change_settings") << "? (y/N): ";
        std::string change;
        std::getline(std::cin, change);
        
        if (change == "y" || change == "Y" || change == "подтверждаю" || change == "да") {
            std::cout << std::endl << "✏️  " << tr("enter_new_settings") << ":" << std::endl;
            
            std::cout << "   " << tr("host") << " [" << currentConfig.host << "]: ";
            std::string host;
            std::getline(std::cin, host);
            if (!host.empty()) currentConfig.host = host;

            std::cout << "   " << tr("port") << " [" << currentConfig.port << "]: ";
            std::string portStr;
            std::getline(std::cin, portStr);
            if (!portStr.empty()) currentConfig.port = std::stoi(portStr);

            std::cout << "   " << tr("database_name") << " [" << currentConfig.database << "]: ";
            std::string db;
            std::getline(std::cin, db);
            if (!db.empty()) currentConfig.database = db;

            std::cout << "   " << tr("username") << " [" << currentConfig.username << "]: ";
            std::string user;
            std::getline(std::cin, user);
            if (!user.empty()) currentConfig.username = user;

            std::cout << "   " << tr("password") << ": ";
            std::string pass;
            std::getline(std::cin, pass);
            if (!pass.empty()) currentConfig.password = pass;

            configManager.saveConfig(currentConfig);
            std::cout << std::endl << "✅ " << tr("settings_saved") << std::endl;
        }

        std::cout << std::endl << "🔍 " << tr("testing_connection") << "..." << std::endl;
        if (dbService.testConnection()) {
            std::cout << "✅ " << tr("connection_success") << std::endl;
            std::cout << "⚙️  " << tr("setting_up_tables") << "..." << std::endl;
            if (dbService.setupDatabase()) {
                std::cout << "✅ " << tr("db_setup_success") << std::endl;
            } else {
                showError(tr("db_setup_error"));
            }
        } else {
            showError(tr("connection_error"));
            std::cout << "💡 " << tr("check_settings") << std::endl;
        }

        waitForEnter();
    }

    // Управление API сервером
    void manageApi() {
        clearScreen();
        drawHeader(tr("api_manage_title"));
        
        if (apiRunning == true) {
            std::cout << "✅ " << tr("api_already_running") << std::endl;
            std::cout << "📍 " << tr("available_at") << ": http://localhost:5000" << std::endl;
            std::cout << std::endl;
            
            std::cout << "🛑 " << tr("stop_api_prompt") << " (y/N): ";
            
            std::string choice;
            std::getline(std::cin, choice);
            
            if (choice == "y" || choice == "Y" || choice == "да" || choice == "д") {
                apiService.stop();
                apiRunning = false;
                std::cout << "✅ " << tr("api_stop_success") << std::endl;
            } else {
                std::cout << "🔵 " << tr("api_keep_running") << std::endl;
            }
        } else {
            std::cout << "🔍 " << tr("checking_db") << "..." << std::endl;
            if (dbService.testConnection()) {
                std::cout << "✅ " << tr("db_available") << std::endl;
                std::cout << "🚀 " << tr("starting_api") << "..." << std::endl;
                
                if (apiService.start()) {
                    apiRunning = true;
                    std::cout << std::endl << "🎉 " << tr("api_start_success") << std::endl;
                    std::cout << "📍 " << tr("available_at") << ": http://localhost:5000" << std::endl;
                    std::cout << std::endl << "📡 " << tr("available_endpoints") << ":" << std::endl;
                    std::cout << "   👥 GET /students   - " << tr("menu_students") << std::endl;
                    std::cout << "   👨‍🏫 GET /teachers  - " << tr("menu_teachers") << std::endl;
                    std::cout << "   🎯 GET /groups     - " << tr("menu_groups") << std::endl;
                } else {
                    showError(tr("api_start_error"));
                }
            } else {
                showError(tr("db_unavailable"));
                std::cout << "💡 " << tr("setup_db_first") << std::endl;
            }
        }

        waitForEnter();
    }

    // Управление студентами
    void manageStudents() {
        clearScreen();
        drawHeader(tr("students_manage_title"));

        auto students = dbService.getStudents();
        
        std::cout << "📊 " << tr("total_students") << ": " << students.size() << std::endl;
        std::cout << std::endl;
        
        if (!students.empty()) {
            std::cout << "┌──────────┬──────────────────┬──────────────────┬──────────────────┬──────────────┬──────────────────────────┬──────────┐" << std::endl;
            std::cout << "│   " << tr("student_code") << "   │     " << tr("last_name") << "     │      " << tr("first_name") << "    │   " << tr("middle_name") << "   │    " << tr("phone") << "   │          " << tr("email") << "           │  " << tr("group") << "  │" << std::endl;
            std::cout << "├──────────┼──────────────────┼──────────────────┼──────────────────┼──────────────┼──────────────────────────┼──────────┤" << std::endl;
            
            for (const auto& student : students) {
                std::cout << "│ " << std::setw(8) << student.studentCode << " │ "
                          << std::setw(16) << std::left << (student.lastName.length() > 16 ? student.lastName.substr(0, 13) + "..." : student.lastName) << " │ "
                          << std::setw(16) << std::left << (student.firstName.length() > 16 ? student.firstName.substr(0, 13) + "..." : student.firstName) << " │ "
                          << std::setw(16) << std::left << (student.middleName.length() > 16 ? student.middleName.substr(0, 13) + "..." : student.middleName) << " │ "
                          << std::setw(12) << std::left << student.phoneNumber << " │ "
                          << std::setw(24) << std::left << (student.email.length() > 24 ? student.email.substr(0, 21) + "..." : student.email) << " │ "
                          << std::setw(8) << student.groupId << " │" << std::endl;
            }
            
            std::cout << "└──────────┴──────────────────┴──────────────────┴──────────────────┴──────────────┴──────────────────────────┴──────────┘" << std::endl;
        } else {
            std::cout << "📭 " << tr("no_students") << std::endl;
        }

        std::cout << std::endl << "💡 " << tr("use_api_hint") << std::endl;
        waitForEnter();
    }

    // Управление преподавателями
    void manageTeachers() {
        clearScreen();
        drawHeader(tr("teachers_manage_title"));

        auto teachers = dbService.getTeachers();
        
        std::cout << "📊 " << tr("total_teachers") << ": " << teachers.size() << std::endl;
        std::cout << std::endl;
        
        if (!teachers.empty()) {
            std::cout << "┌──────────┬──────────────────┬──────────────────┬──────────────────┬──────────┬──────────────────┬──────────────────────────┬──────────────┐" << std::endl;
            std::cout << "│    " << tr("teacher_id") << "   │     " << tr("last_name") << "     │      " << tr("first_name") << "    │   " << tr("middle_name") << "   │   " << tr("experience") << "  │ " << tr("specialization") << "   │          " << tr("email") << "           │    " << tr("phone") << "   │" << std::endl;
            std::cout << "├──────────┼──────────────────┼──────────────────┼──────────────────┼──────────┼──────────────────┼──────────────────────────┼──────────────┤" << std::endl;
            
            for (const auto& teacher : teachers) {
                std::cout << "│ " << std::setw(8) << teacher.teacherId << " │ "
                          << std::setw(16) << std::left << teacher.lastName << " │ "
                          << std::setw(16) << std::left << teacher.firstName << " │ "
                          << std::setw(16) << std::left << teacher.middleName << " │ "
                          << std::setw(8) << teacher.experience << " │ "
                          << std::setw(16) << std::left << (teacher.specialization.length() > 16 ? teacher.specialization.substr(0, 13) + "..." : teacher.specialization) << " │ "
                          << std::setw(24) << std::left << (teacher.email.length() > 24 ? teacher.email.substr(0, 21) + "..." : teacher.email) << " │ "
                          << std::setw(12) << std::left << teacher.phoneNumber << " │" << std::endl;
            }
            
            std::cout << "└──────────┴──────────────────┴──────────────────┴──────────────────┴──────────┴──────────────────┴──────────────────────────┴──────────────┘" << std::endl;
        } else {
            std::cout << "📭 " << tr("no_teachers") << std::endl;
        }

        waitForEnter();
    }

    // Управление группами
    void manageGroups() {
        clearScreen();
        drawHeader(tr("groups_manage_title"));

        auto groups = dbService.getGroups();
        
        std::cout << "📊 " << tr("total_groups") << ": " << groups.size() << std::endl;
        std::cout << std::endl;
        
        if (!groups.empty()) {
            std::cout << "┌──────────┬──────────────────────┬──────────────────┬──────────────┐" << std::endl;
            std::cout << "│    " << tr("group_name") << "   │       " << tr("group_name") << "      │ " << tr("student_count") << " │  " << tr("teacher_id") << "  │" << std::endl;
            std::cout << "├──────────┼──────────────────────┼──────────────────┼──────────────┤" << std::endl;
            
            for (const auto& group : groups) {
                std::cout << "│ " << std::setw(8) << group.groupId << " │ "
                          << std::setw(20) << std::left << group.name << " │ "
                          << std::setw(16) << std::left << group.studentCount << " │ "
                          << std::setw(12) << std::left << group.teacherId << " │" << std::endl;
            }
            
            std::cout << "└──────────┴──────────────────────┴──────────────────┴──────────────┘" << std::endl;
        } else {
            std::cout << "📭 " << tr("no_groups") << std::endl;
        }

        waitForEnter();
    }

    // Управление портфолио
    void managePortfolios() {
        clearScreen();
        drawHeader(tr("portfolios_manage_title"));

        auto portfolios = dbService.getPortfolios();
        
        std::cout << "📊 " << tr("total_portfolios") << ": " << portfolios.size() << std::endl;
        std::cout << std::endl;
        
        if (!portfolios.empty()) {
            std::cout << "┌──────────────┬──────────────┬────────────────────┬────────────┬────────────────┬────────────────┐" << std::endl;
            std::cout << "│   " << tr("portfolio_id") << "  │  " << tr("student_id") << " │   " << tr("measure_code") << "   │    " << tr("date") << "    │ " << tr("passport_series") << " │ " << tr("passport_number") << " │" << std::endl;
            std::cout << "├──────────────┼──────────────┼────────────────────┼────────────┼────────────────┼────────────────┤" << std::endl;
            
            for (const auto& portfolio : portfolios) {
                std::cout << "│ " << std::setw(12) << portfolio.portfolioId << " │ "
                          << std::setw(12) << std::left << portfolio.studentCode << " │ "
                          << std::setw(18) << std::left << portfolio.measureCode << " │ "
                          << std::setw(10) << std::left << portfolio.date << " │ "
                          << std::setw(14) << std::left << portfolio.passportSeries << " │ "
                          << std::setw(14) << std::left << portfolio.passportNumber << " │" << std::endl;
            }
            
            std::cout << "└──────────────┴──────────────┴────────────────────┴────────────┴────────────────┴────────────────┘" << std::endl;
        } else {
            std::cout << "📭 " << tr("no_portfolios") << std::endl;
        }

        waitForEnter();
    }

    // Информация о системе
    void showSystemInfo() {
        clearScreen();
        drawHeader(tr("system_info_title"));
        
        std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                 Student Management System                ║" << std::endl;
        std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
        std::cout << "║  🎯 " << tr("app_title") << ": 1.0                                  ║" << std::endl;
        std::cout << "║  🖥️  Platform: Windows/Linux                            ║" << std::endl;
        std::cout << "║  🗄️  Database: PostgreSQL 12+                           ║" << std::endl;
        std::cout << "║  💻 Programming Language: C++17                         ║" << std::endl;
        std::cout << "║  📚 Used Libraries:                                     ║" << std::endl;
        std::cout << "║     • libpq (PostgreSQL client)                         ║" << std::endl;
        std::cout << "║     • nlohmann/json (JSON processing)                   ║" << std::endl;
        std::cout << "║                                                          ║" << std::endl;
        std::cout << "║  🚀 Main Features:                                      ║" << std::endl;
        std::cout << "║     • " << tr("menu_students") << "                   ║" << std::endl;
        std::cout << "║     • " << tr("menu_teachers") << "                ║" << std::endl;
        std::cout << "║     • " << tr("menu_groups") << "                      ║" << std::endl;
        std::cout << "║     • " << tr("menu_portfolios") << "                   ║" << std::endl;
        std::cout << "║     • REST API for integration                          ║" << std::endl;
        std::cout << "║     • Cross-platform                                    ║" << std::endl;
        std::cout << "║                                                          ║" << std::endl;
        std::cout << "║  📞 Developer: Dmitry Stolbov                           ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;

        waitForEnter();
    }

    // Выход из приложения
    void exitApplication() {
        clearScreen();
        drawHeader(tr("exit_title"));
        
        if (apiRunning) {
            std::cout << "🛑 " << tr("stopping_api") << "..." << std::endl;
            apiService.stop();
            apiRunning = false;
            std::cout << "✅ " << tr("api_stop_success") << std::endl;
        }
        
        std::cout << std::endl << "👋 " << tr("thank_you") << std::endl;
        std::cout << std::endl;
    }

    // Вспомогательные методы
    void waitForEnter() {
        std::cout << std::endl << "↵ " << tr("press_enter") << std::endl;
        std::cin.get();
    }

    void showError(const std::string& message) {
        std::cout << "❌ " << tr("error") << ": " << message << std::endl;
    }

    void showSuccess(const std::string& message) {
        std::cout << "✅ " << message << std::endl;
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
        std::cout << "🌍 Checking localization files..." << std::endl;
        if (!LocaleManager::checkLocales()) {
            std::cout << "❌ Localization files missing. Please make sure lang/locale_en.json and lang/locale_ru.json exist." << std::endl;
            return 1;
        }

        // Загружаем конфигурацию для определения языка
        ConfigManager configManager;
        DatabaseConfig config;
        configManager.loadConfig(config);
        
        // Загружаем локализацию на основе конфигурации
        std::map<std::string, std::string> currentLocale = LocaleManager::loadLocale(config.language);
        
        // Если не удалось загрузить локализацию, выходим
        if (currentLocale.empty()) {
            std::cerr << "❌ Failed to load localization for language: " << config.language << std::endl;
            return 1;
        }

        std::cout << std::endl << "🚀 Starting application..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        Application app(currentLocale);
        app.showMainMenu();
    } catch (const std::exception& e) {
        std::cerr << "💥 Critical error: " << e.what() << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    return 0;
}