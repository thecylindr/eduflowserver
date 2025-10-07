#include <iostream>
#include <string>
#include <cstdlib>
#include <limits>
#include <iomanip>
#include "DatabaseService.h"
#include "ApiService.h"
#include "ConfigManager.h"

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

public:
    Application() : apiService(dbService) {}

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

    // Отображает главное меню
    void showMainMenu() {
        while (true) {
            clearScreen();
            drawHeader("СТУДЕНЧЕСКАЯ БАЗА ДАННЫХ");
            
            // Статус системы
            std::cout << "📊 СТАТУС СИСТЕМЫ:" << std::endl;
            std::cout << "   🗄️  База данных: " << (dbService.testConnection() ? "✅ Подключена" : "❌ Отключена") << std::endl;
            std::cout << "   🌐 API сервер: " << (apiRunning ? "✅ Запущен" : "❌ Остановлен") << std::endl;
            std::cout << std::endl;
            
            drawSeparator();
            
            std::cout << "📋 ГЛАВНОЕ МЕНЮ:" << std::endl;
            std::cout << std::endl;
            
            std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
            std::cout << "║  1. ⚙️   Настройка базы данных                          ║" << std::endl;
            std::cout << "║  2. 🌐  Управление API сервером                         ║" << std::endl;
            std::cout << "║  3. 👥  Управление студентами                           ║" << std::endl;
            std::cout << "║  4. 👨‍🏫 Управление преподавателями                      ║" << std::endl;
            std::cout << "║  5. 🎯 Управление группами                              ║" << std::endl;
            std::cout << "║  6. 📁 Управление портфолио                             ║" << std::endl;
            std::cout << "║  7. ℹ️   Информация о системе                           ║" << std::endl;
            std::cout << "║                                                          ║" << std::endl;
            std::cout << "║  Q. 🚪 Выйти из программы                               ║" << std::endl;
            std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
            
            std::cout << std::endl << "🎯 Выберите опцию: ";
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
            } else if (choice == "Q" || choice == "q") {
                exitApplication();
                break;
            } else {
                showError("Неверный выбор!");
            }
        }
    }

private:
    // Настройка базы данных
    void setupDatabase() {
        clearScreen();
        drawHeader("НАСТРОЙКА БАЗЫ ДАННЫХ");

        DatabaseConfig currentConfig = dbService.getCurrentConfig();
        
        std::cout << "📄 Текущие настройки из config.json:" << std::endl;
        std::cout << "   📍 Хост: " << currentConfig.host << std::endl;
        std::cout << "   🚪 Порт: " << currentConfig.port << std::endl;
        std::cout << "   🗄️  База данных: " << currentConfig.database << std::endl;
        std::cout << "   👤 Пользователь: " << currentConfig.username << std::endl;
        std::cout << "   🔒 Пароль: " << std::string(currentConfig.password.length(), '*') << std::endl;
        
        std::cout << std::endl << "🔄 Хотите изменить настройки? (y/N): ";
        std::string change;
        std::getline(std::cin, change);
        
        if (change == "y" || change == "Y") {
            std::cout << std::endl << "✏️  Введите новые настройки:" << std::endl;
            
            std::cout << "   Хост [" << currentConfig.host << "]: ";
            std::string host;
            std::getline(std::cin, host);
            if (!host.empty()) currentConfig.host = host;

            std::cout << "   Порт [" << currentConfig.port << "]: ";
            std::string portStr;
            std::getline(std::cin, portStr);
            if (!portStr.empty()) currentConfig.port = std::stoi(portStr);

            std::cout << "   База данных [" << currentConfig.database << "]: ";
            std::string db;
            std::getline(std::cin, db);
            if (!db.empty()) currentConfig.database = db;

            std::cout << "   Пользователь [" << currentConfig.username << "]: ";
            std::string user;
            std::getline(std::cin, user);
            if (!user.empty()) currentConfig.username = user;

            std::cout << "   Пароль: ";
            std::string pass;
            std::getline(std::cin, pass);
            if (!pass.empty()) currentConfig.password = pass;

            configManager.saveConfig(currentConfig);
            std::cout << std::endl << "✅ Настройки сохранены в config.json" << std::endl;
        }

        std::cout << std::endl << "🔍 Проверка подключения к БД..." << std::endl;
        if (dbService.testConnection()) {
            std::cout << "✅ Подключение успешно!" << std::endl;
            std::cout << "⚙️  Настройка таблиц..." << std::endl;
            if (dbService.setupDatabase()) {
                std::cout << "✅ База данных настроена!" << std::endl;
            } else {
                showError("Ошибка настройки базы данных!");
            }
        } else {
            showError("Ошибка подключения к базе данных!");
            std::cout << "💡 Проверьте настройки в config.json и убедитесь, что PostgreSQL запущен." << std::endl;
        }

        waitForEnter();
    }

    // Управление API сервером
    void manageApi() {
        clearScreen();
        drawHeader("УПРАВЛЕНИЕ API СЕРВЕРОМ");
        
        if (!apiRunning) {
            std::cout << "🔍 Проверка подключения к базе данных..." << std::endl;
            if (dbService.testConnection()) {
                std::cout << "✅ База данных доступна" << std::endl;
                std::cout << "🚀 Запуск API сервера..." << std::endl;
                
                if (apiService.start()) {
                    apiRunning = true;
                    std::cout << std::endl << "🎉 API запущено успешно!" << std::endl;
                    std::cout << "📍 Доступно по адресу: http://localhost:5000" << std::endl;
                    std::cout << std::endl << "📡 Доступные endpoints:" << std::endl;
                    std::cout << "   👥 GET /students   - список студентов" << std::endl;
                    std::cout << "   👨‍🏫 GET /teachers  - список преподавателей" << std::endl;
                    std::cout << "   🎯 GET /groups     - список групп" << std::endl;
                } else {
                    showError("Ошибка запуска API!");
                }
            } else {
                showError("База данных недоступна!");
                std::cout << "💡 Сначала настройте подключение к базе данных." << std::endl;
            }
        } else {
            apiService.stop();
            apiRunning = false;
            std::cout << "✅ API сервер остановлен" << std::endl;
        }

        waitForEnter();
    }

    // Управление студентами
    void manageStudents() {
        clearScreen();
        drawHeader("УПРАВЛЕНИЕ СТУДЕНТАМИ");

        auto students = dbService.getStudents();
        
        std::cout << "📊 Всего студентов: " << students.size() << std::endl;
        std::cout << std::endl;
        
        if (!students.empty()) {
            std::cout << "┌──────────┬──────────────────┬──────────────────┬──────────────────┬──────────────┬──────────────────────────┬──────────┐" << std::endl;
            std::cout << "│   Код    │     Фамилия      │      Имя         │   Отчество       │    Телефон   │          Email           │  Группа  │" << std::endl;
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
            std::cout << "📭 Студенты не найдены" << std::endl;
        }

        std::cout << std::endl << "💡 Для добавления студентов используйте API или прямые SQL запросы" << std::endl;
        waitForEnter();
    }

    // Управление преподавателями
    void manageTeachers() {
        clearScreen();
        drawHeader("УПРАВЛЕНИЕ ПРЕПОДАВАТЕЛЯМИ");

        auto teachers = dbService.getTeachers();
        
        std::cout << "📊 Всего преподавателей: " << teachers.size() << std::endl;
        std::cout << std::endl;
        
        if (!teachers.empty()) {
            std::cout << "┌──────────┬──────────────────┬──────────────────┬──────────────────┬──────────┬──────────────────┬──────────────────────────┬──────────────┐" << std::endl;
            std::cout << "│    ID    │     Фамилия      │      Имя         │   Отчество       │   Стаж   │ Специализация    │          Email           │    Телефон   │" << std::endl;
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
            std::cout << "📭 Преподаватели не найдены" << std::endl;
        }

        waitForEnter();
    }

    // Управление группами
    void manageGroups() {
        clearScreen();
        drawHeader("УПРАВЛЕНИЕ ГРУППАМИ");

        auto groups = dbService.getGroups();
        
        std::cout << "📊 Всего групп: " << groups.size() << std::endl;
        std::cout << std::endl;
        
        if (!groups.empty()) {
            std::cout << "┌──────────┬──────────────────────┬──────────────────┬──────────────┐" << std::endl;
            std::cout << "│    ID    │       Название       │ Кол-во студентов │  ID учителя  │" << std::endl;
            std::cout << "├──────────┼──────────────────────┼──────────────────┼──────────────┤" << std::endl;
            
            for (const auto& group : groups) {
                std::cout << "│ " << std::setw(8) << group.groupId << " │ "
                          << std::setw(20) << std::left << group.name << " │ "
                          << std::setw(16) << std::left << group.studentCount << " │ "
                          << std::setw(12) << std::left << group.teacherId << " │" << std::endl;
            }
            
            std::cout << "└──────────┴──────────────────────┴──────────────────┴──────────────┘" << std::endl;
        } else {
            std::cout << "📭 Группы не найдены" << std::endl;
        }

        waitForEnter();
    }

    // Управление портфолио
    void managePortfolios() {
        clearScreen();
        drawHeader("УПРАВЛЕНИЕ ПОРТФОЛИО");

        auto portfolios = dbService.getPortfolios();
        
        std::cout << "📊 Всего портфолио: " << portfolios.size() << std::endl;
        std::cout << std::endl;
        
        if (!portfolios.empty()) {
            std::cout << "┌──────────────┬──────────────┬────────────────────┬────────────┬────────────────┬────────────────┐" << std::endl;
            std::cout << "│   ID портф.  │  ID студента │   Код измерения    │    Дата    │ Серия паспорта │ Номер паспорта │" << std::endl;
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
            std::cout << "📭 Портфолио не найдены" << std::endl;
        }

        waitForEnter();
    }

    // Информация о системе
    void showSystemInfo() {
        clearScreen();
        drawHeader("ИНФОРМАЦИЯ О СИСТЕМЕ");
        
        std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                 Student Management System                ║" << std::endl;
        std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
        std::cout << "║  🎯 Версия: 1.0                                          ║" << std::endl;
        std::cout << "║  🖥️  Поддержка: Windows/Linux                            ║" << std::endl;
        std::cout << "║  🗄️  База данных: PostgreSQL 12+                         ║" << std::endl;
        std::cout << "║  💻 Язык программирования: C++17                         ║" << std::endl;
        std::cout << "║  📚 Используемые библиотеки:                             ║" << std::endl;
        std::cout << "║     • libpq (PostgreSQL client)                          ║" << std::endl;
        std::cout << "║     • nlohmann/json (JSON processing)                    ║" << std::endl;
        std::cout << "║                                                          ║" << std::endl;
        std::cout << "║  🚀 Основные функции:                                    ║" << std::endl;
        std::cout << "║     • Управление студентами и группами                   ║" << std::endl;
        std::cout << "║     • Управление преподавателями                         ║" << std::endl;
        std::cout << "║     • Ведение портфолио студентов                        ║" << std::endl;
        std::cout << "║     • REST API для интеграции                            ║" << std::endl;
        std::cout << "║     • Кросс-платформенность                              ║" << std::endl;
        std::cout << "║                                                          ║" << std::endl;
        std::cout << "║  📞 Разработчик: Столбов Дмитрий Олегович                ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;

        waitForEnter();
    }

    // Выход из приложения
    void exitApplication() {
        clearScreen();
        drawHeader("ВЫХОД ИЗ ПРОГРАММЫ");
        
        if (apiRunning) {
            std::cout << "🛑 Остановка API сервера..." << std::endl;
            apiService.stop();
            apiRunning = false;
            std::cout << "✅ API сервер остановлен" << std::endl;
        }
        
        std::cout << std::endl << "👋 Спасибо за использование системы! До свидания!" << std::endl;
        std::cout << std::endl;
    }

    // Вспомогательные методы
    void waitForEnter() {
        std::cout << std::endl << "↵ Нажмите Enter для продолжения..." << std::endl;
        std::cin.get();
    }

    void showError(const std::string& message) {
        std::cout << "❌ Ошибка: " << message << std::endl;
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
        Application app;
        app.showMainMenu();
    } catch (const std::exception& e) {
        std::cerr << "💥 Критическая ошибка: " << e.what() << std::endl;
        std::cout << "↵ Нажмите Enter для выхода..." << std::endl;
        std::cin.get();
        return 1;
    }

    return 0;
}