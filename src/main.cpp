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
    
    // Фоновые цвета
    const std::string BG_BLUE = "\033[44m";
    const std::string BG_GREEN = "\033[42m";
    const std::string BG_RED = "\033[41m";
    const std::string BG_YELLOW = "\033[43m";
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
        return key;
    }

    // Очистка экрана
    void clearScreen() {
        system(CLEAR_SCREEN);
    }

    // Красивый заголовок с цветом
    void drawHeader(const std::string& title) {
        std::cout << "┌────────────────────────────────────────────────────────────┐" << std::endl;
        std::cout << "                🎓 " << title << " 🎓                  " << std::endl;
        std::cout << "└────────────────────────────────────────────────────────────┘" << std::endl;
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
        
        locale = LocaleManager::loadLocale(newLanguage);
        
        DatabaseConfig config = dbService.getCurrentConfig();
        config.language = newLanguage;
        configManager.saveConfig(config);
        
        showSuccess(tr("language_changed"));
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
            std::cout << Colors::CYAN << "1. ⚙️ " << tr("menu_db_setup") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "2. 🌐 " << tr("menu_api_manage") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "3. 👥 " << tr("menu_students") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "4. 🏫 " << tr("menu_teachers") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "5. 🎯 " << tr("menu_groups") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "6. 📁 " << tr("menu_portfolios") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "7. ℹ️  " << tr("menu_system_info") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "8. 🌍 " << tr("menu_change_language") << Colors::RESET << std::endl;
            std::cout << Colors::RED << "Q. 🚪 " << tr("menu_exit") << Colors::RESET << std::endl;
            
            std::cout << std::endl << Colors::YELLOW << "🎯 " << tr("choose_option") << ": " << Colors::RESET;
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
        
        std::cout << Colors::MAGENTA << "📄 " << tr("current_settings") << ":" << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   📍 " << tr("host") << ": " << Colors::WHITE << currentConfig.host << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   🚪 " << tr("port") << ": " << Colors::WHITE << currentConfig.port << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   🗄️ " << tr("database_name") << ": " << Colors::WHITE << currentConfig.database << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   👤 " << tr("username") << ": " << Colors::WHITE << currentConfig.username << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   🔒 " << tr("password") << ": " << Colors::WHITE << std::string(currentConfig.password.length(), '*') << Colors::RESET << std::endl;
        
        std::cout << std::endl << Colors::YELLOW << "🔄 " << tr("change_settings") << "? (y/N): " << Colors::RESET;
        std::string change;
        std::getline(std::cin, change);
        
        if (change == "y" || change == "Y" || change == "да") {
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

            configManager.saveConfig(currentConfig);
            showSuccess(tr("settings_saved"));
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

    // Управление API сервером
    void manageApi() {
        clearScreen();
        drawHeader(tr("api_manage_title"));
        
        if (apiRunning) {
            showSuccess(tr("api_already_running"));
            std::cout << Colors::CYAN << "📍 " << tr("available_at") << ": " << Colors::WHITE << "http://localhost:5000" << Colors::RESET << std::endl;
            std::cout << std::endl;
            
            std::cout << Colors::YELLOW << "🛑 " << tr("stop_api_prompt") << " (y/N): " << Colors::RESET;
            
            std::string choice;
            std::getline(std::cin, choice);
            
            if (choice == "y" || choice == "Y" || choice == "да" || choice == "д") {
                apiService.stop();
                apiRunning = false;
                showSuccess(tr("api_stop_success"));
            } else {
                showInfo(tr("api_keep_running"));
            }
        } else {
            std::cout << Colors::YELLOW << "🔍 " << tr("checking_db") << "..." << Colors::RESET << std::endl;
            if (dbService.testConnection()) {
                showSuccess(tr("db_available"));
                std::cout << Colors::YELLOW << "🚀 " << tr("starting_api") << "..." << Colors::RESET << std::endl;
                
                if (apiService.start()) {
                    apiRunning = true;
                    std::cout << std::endl;
                    showSuccess(tr("api_start_success"));
                    std::cout << Colors::CYAN << "📍 " << tr("available_at") << ": " << Colors::WHITE << "http://localhost:5000" << Colors::RESET << std::endl;
                    std::cout << std::endl << Colors::MAGENTA << "📡 " << tr("available_endpoints") << ":" << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   👥 GET /students   - " << Colors::WHITE << tr("menu_students") << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   👨‍🏫 GET /teachers  - " << Colors::WHITE << tr("menu_teachers") << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   🎯 GET /groups     - " << Colors::WHITE << tr("menu_groups") << Colors::RESET << std::endl;
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

    // Управление студентами
    void manageStudents() {
        clearScreen();
        drawHeader(tr("students_manage_title"));

        auto students = dbService.getStudents();
        
        std::cout << Colors::MAGENTA << "📊 " << tr("total_students") << ": " << Colors::YELLOW << students.size() << Colors::RESET << std::endl;
        std::cout << std::endl;
        
        if (!students.empty()) {
            for (const auto& student : students) {
                std::cout << Colors::CYAN << "👤 " << tr("student") << " " << student.studentCode << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   📍 " << tr("last_name") << ": " << Colors::WHITE << student.lastName << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   📍 " << tr("first_name") << ": " << Colors::WHITE << student.firstName << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   📍 " << tr("middle_name") << ": " << Colors::WHITE << student.middleName << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   📞 " << tr("phone") << ": " << Colors::WHITE << student.phoneNumber << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   ✉️  " << tr("email") << ": " << Colors::WHITE << student.email << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   🎯 " << tr("group") << ": " << Colors::WHITE << student.groupId << Colors::RESET << std::endl;
                std::cout << std::endl;
            }
        } else {
            showWarning(tr("no_students"));
        }

        showInfo(tr("use_api_hint"));
        waitForEnter();
    }

    // Управление преподавателями
    void manageTeachers() {
        while (true) {
            std::cout << Colors::YELLOW << "\n👨‍🏫 " << tr("teacher_management") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "1. " << tr("view_teachers") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "2. " << tr("add_teacher") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "3. " << tr("edit_teacher") << Colors::RESET << std::endl;
            std::cout << Colors::CYAN << "4. " << tr("delete_teacher") << Colors::RESET << std::endl;
            std::cout << Colors::RED << "5. " << tr("back") << Colors::RESET << std::endl;
            
            std::cout << Colors::GREEN << "\n➡️ " << tr("enter_choice") << ": " << Colors::RESET;
            std::string choice;
            std::getline(std::cin, choice);
            
            if (choice == "1") {
                // Просмотр преподавателей
                auto teachers = dbService.getTeachers();
                std::cout << Colors::YELLOW << "\n📋 " << tr("teachers_list") << " (" << teachers.size() << "):" << Colors::RESET << std::endl;
                
                for (const auto& teacher : teachers) {
                    std::cout << Colors::GREEN << "👨‍🏫 " << teacher.lastName << " " << teacher.firstName << " " << teacher.middleName << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   🆔 " << tr("id") << ": " << Colors::WHITE << teacher.teacherId << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   📧 " << tr("email") << ": " << Colors::WHITE << teacher.email << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   📞 " << tr("phone") << ": " << Colors::WHITE << teacher.phoneNumber << Colors::RESET << std::endl;
                    std::cout << Colors::CYAN << "   📊 " << tr("experience") << ": " << Colors::WHITE << teacher.experience << " " << tr("years") << Colors::RESET << std::endl;
                    
                    // Исправленный вывод специализаций
                    std::cout << Colors::CYAN << "   🎯 " << tr("specializations") << ": " << Colors::WHITE;
                    auto specializations = dbService.getTeacherSpecializations(teacher.teacherId);
                    if (specializations.empty()) {
                        std::cout << tr("none");
                    } else {
                        for (size_t i = 0; i < specializations.size(); ++i) {
                            std::cout << specializations[i].name; // Теперь это Specialization, а не string
                            if (i < specializations.size() - 1) {
                                std::cout << ", ";
                            }
                        }
                    }
                    std::cout << Colors::RESET << std::endl;
                    std::cout << std::endl;
                }
                
            } else if (choice == "2") {
                // Добавление преподавателя
                std::cout << Colors::YELLOW << "\n➕ " << tr("add_teacher") << Colors::RESET << std::endl;
                
                Teacher teacher;
                std::cout << Colors::GREEN << tr("enter_last_name") << ": " << Colors::RESET;
                std::getline(std::cin, teacher.lastName);
                
                std::cout << Colors::GREEN << tr("enter_first_name") << ": " << Colors::RESET;
                std::getline(std::cin, teacher.firstName);
                
                std::cout << Colors::GREEN << tr("enter_middle_name") << ": " << Colors::RESET;
                std::getline(std::cin, teacher.middleName);
                
                std::cout << Colors::GREEN << tr("enter_experience") << ": " << Colors::RESET;
                std::string expStr;
                std::getline(std::cin, expStr);
                teacher.experience = expStr.empty() ? 0 : std::stoi(expStr);
                
                std::cout << Colors::GREEN << tr("enter_email") << ": " << Colors::RESET;
                std::getline(std::cin, teacher.email);
                
                std::cout << Colors::GREEN << tr("enter_phone") << ": " << Colors::RESET;
                std::getline(std::cin, teacher.phoneNumber);
                
                if (dbService.addTeacher(teacher)) {
                    std::cout << Colors::GREEN << "✅ " << tr("teacher_added_success") << Colors::RESET << std::endl;
                    
                    // Предложение добавить специализации
                    std::cout << Colors::CYAN << tr("add_specializations_prompt") << " (y/n): " << Colors::RESET;
                    std::string addSpecs;
                    std::getline(std::cin, addSpecs);
                    
                    if (addSpecs == "y" || addSpecs == "Y") {
                        auto allSpecs = dbService.getSpecializations();
                        if (!allSpecs.empty()) {
                            std::cout << Colors::YELLOW << "\n📚 " << tr("available_specializations") << ":" << Colors::RESET << std::endl;
                            for (const auto& spec : allSpecs) {
                                std::cout << Colors::CYAN << "   " << spec.specializationCode << ". " << spec.name << Colors::RESET << std::endl;
                            }
                            
                            std::cout << Colors::GREEN << tr("enter_specialization_codes") << " (comma-separated): " << Colors::RESET;
                            std::string codesInput;
                            std::getline(std::cin, codesInput);
                            
                            // Парсим коды специализаций
                            std::stringstream ss(codesInput);
                            std::string codeStr;
                            while (std::getline(ss, codeStr, ',')) {
                                if (!codeStr.empty()) {
                                    try {
                                        int code = std::stoi(codeStr);
                                        dbService.addTeacherSpecialization(teacher.teacherId, code);
                                    } catch (const std::exception& e) {
                                        std::cout << Colors::RED << "❌ " << tr("invalid_code") << ": " << codeStr << Colors::RESET << std::endl;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    std::cout << Colors::RED << "❌ " << tr("teacher_add_failed") << Colors::RESET << std::endl;
                }
                
            } else if (choice == "3") {
                // Редактирование преподавателя
                std::cout << Colors::YELLOW << "\n✏️ " << tr("edit_teacher") << Colors::RESET << std::endl;
                std::cout << Colors::GREEN << tr("enter_teacher_id") << ": " << Colors::RESET;
                std::string idStr;
                std::getline(std::cin, idStr);
                
                int teacherId = std::stoi(idStr);
                Teacher teacher = dbService.getTeacherById(teacherId);
                
                if (teacher.teacherId == 0) {
                    std::cout << Colors::RED << "❌ " << tr("teacher_not_found") << Colors::RESET << std::endl;
                    continue;
                }
                
                std::cout << Colors::CYAN << tr("editing_teacher") << ": " << teacher.lastName << " " << teacher.firstName << Colors::RESET << std::endl;
                
                std::cout << Colors::GREEN << tr("enter_last_name") << " (" << teacher.lastName << "): " << Colors::RESET;
                std::string lastName;
                std::getline(std::cin, lastName);
                if (!lastName.empty()) teacher.lastName = lastName;
                
                std::cout << Colors::GREEN << tr("enter_first_name") << " (" << teacher.firstName << "): " << Colors::RESET;
                std::string firstName;
                std::getline(std::cin, firstName);
                if (!firstName.empty()) teacher.firstName = firstName;
                
                std::cout << Colors::GREEN << tr("enter_middle_name") << " (" << teacher.middleName << "): " << Colors::RESET;
                std::string middleName;
                std::getline(std::cin, middleName);
                if (!middleName.empty()) teacher.middleName = middleName;
                
                std::cout << Colors::GREEN << tr("enter_experience") << " (" << teacher.experience << "): " << Colors::RESET;
                std::string expStr;
                std::getline(std::cin, expStr);
                if (!expStr.empty()) teacher.experience = std::stoi(expStr);
                
                std::cout << Colors::GREEN << tr("enter_email") << " (" << teacher.email << "): " << Colors::RESET;
                std::string email;
                std::getline(std::cin, email);
                if (!email.empty()) teacher.email = email;
                
                std::cout << Colors::GREEN << tr("enter_phone") << " (" << teacher.phoneNumber << "): " << Colors::RESET;
                std::string phone;
                std::getline(std::cin, phone);
                if (!phone.empty()) teacher.phoneNumber = phone;
                
                if (dbService.updateTeacher(teacher)) {
                    std::cout << Colors::GREEN << "✅ " << tr("teacher_updated_success") << Colors::RESET << std::endl;
                } else {
                    std::cout << Colors::RED << "❌ " << tr("teacher_update_failed") << Colors::RESET << std::endl;
                }
                
            } else if (choice == "4") {
                // Удаление преподавателя
                std::cout << Colors::YELLOW << "\n🗑️ " << tr("delete_teacher") << Colors::RESET << std::endl;
                std::cout << Colors::GREEN << tr("enter_teacher_id") << ": " << Colors::RESET;
                std::string idStr;
                std::getline(std::cin, idStr);
                
                int teacherId = std::stoi(idStr);
                
                std::cout << Colors::RED << tr("confirm_delete") << " (y/n): " << Colors::RESET;
                std::string confirm;
                std::getline(std::cin, confirm);
                
                if (confirm == "y" || confirm == "Y") {
                    if (dbService.deleteTeacher(teacherId)) {
                        std::cout << Colors::GREEN << "✅ " << tr("teacher_deleted_success") << Colors::RESET << std::endl;
                    } else {
                        std::cout << Colors::RED << "❌ " << tr("teacher_delete_failed") << Colors::RESET << std::endl;
                    }
                }
                
            } else if (choice == "5") {
                break;
            } else {
                std::cout << Colors::RED << "❌ " << tr("invalid_choice") << Colors::RESET << std::endl;
            }
        }
    }

    // Управление группами
    void manageGroups() {
        clearScreen();
        drawHeader(tr("groups_manage_title"));

        auto groups = dbService.getGroups();
        
        std::cout << Colors::MAGENTA << "📊 " << tr("total_groups") << ": " << Colors::YELLOW << groups.size() << Colors::RESET << std::endl;
        std::cout << std::endl;
        
        if (!groups.empty()) {
            for (const auto& group : groups) {
                std::cout << Colors::CYAN << "🎯 " << tr("group") << " " << group.groupId << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   📝 " << tr("group_name") << ": " << Colors::WHITE << group.name << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   👥 " << tr("student_count") << ": " << Colors::WHITE << group.studentCount << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   👨‍🏫 " << tr("teacher_id") << ": " << Colors::WHITE << group.teacherId << Colors::RESET << std::endl;
                std::cout << std::endl;
            }
        } else {
            showWarning(tr("no_groups"));
        }

        waitForEnter();
    }

    // Управление портфолио
    void managePortfolios() {
        clearScreen();
        drawHeader(tr("portfolios_manage_title"));

        auto portfolios = dbService.getPortfolios();
        
        std::cout << Colors::MAGENTA << "📊 " << tr("total_portfolios") << ": " << Colors::YELLOW << portfolios.size() << Colors::RESET << std::endl;
        std::cout << std::endl;
        
        if (!portfolios.empty()) {
            for (const auto& portfolio : portfolios) {
                std::cout << Colors::CYAN << "📁 " << tr("portfolio") << " " << portfolio.portfolioId << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   👤 " << tr("student_id") << ": " << Colors::WHITE << portfolio.studentCode << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   📊 " << tr("measure_code") << ": " << Colors::WHITE << portfolio.measureCode << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   📅 " << tr("date") << ": " << Colors::WHITE << portfolio.date << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   🆔 " << tr("passport_series") << ": " << Colors::WHITE << portfolio.passportSeries << Colors::RESET << std::endl;
                std::cout << Colors::CYAN << "   🔢 " << tr("passport_number") << ": " << Colors::WHITE << portfolio.passportNumber << Colors::RESET << std::endl;
                std::cout << std::endl;
            }
        } else {
            showWarning(tr("no_portfolios"));
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
        std::cout << Colors::CYAN << "   • " << Colors::WHITE << tr("menu_students") << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • " << Colors::WHITE << tr("menu_teachers") << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • " << Colors::WHITE << tr("menu_groups") << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • " << Colors::WHITE << tr("menu_portfolios") << Colors::RESET << std::endl;
        std::cout << Colors::CYAN << "   • " << Colors::WHITE << "REST API for integration" << Colors::RESET << std::endl;
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
        configManager.loadConfig(config);
        
        // Загружаем локализацию на основе конфигурации
        std::map<std::string, std::string> currentLocale = LocaleManager::loadLocale(config.language);
        
        if (currentLocale.empty()) {
            std::cerr << Colors::RED << "❌ Failed to load localization for language: " << config.language << Colors::RESET << std::endl;
            return 1;
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