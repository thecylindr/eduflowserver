#include "DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>

// Конструктор - загружаем конфигурацию при создании объекта
DatabaseService::DatabaseService() : connection(nullptr) {
    configManager.loadConfig(currentConfig);
}

// Деструктор - отключаемся от БД при уничтожении объекта
DatabaseService::~DatabaseService() {
    disconnect();
}

// Подключение к базе данных с использованием конфигурации
bool DatabaseService::connect(const DatabaseConfig& config) {
    // Если уже подключены - отключаемся сначала
    if (connection) {
        disconnect();
    }
    
    currentConfig = config;
    
    // Формируем строку подключения для PostgreSQL
    std::string connStr = "host=" + config.host + 
                         " port=" + std::to_string(config.port) + 
                         " dbname=" + config.database + 
                         " user=" + config.username + 
                         " password=" + config.password;
    
    connection = PQconnectdb(connStr.c_str());
    
    // Проверяем успешность подключения
    if (PQstatus(connection) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(connection) << std::endl;
        PQfinish(connection);
        connection = nullptr;
        return false;
    }
    
    std::cout << "Connected to PostgreSQL successfully!" << std::endl;
    return true;
}

// Отключение от базы данных
void DatabaseService::disconnect() {
    if (connection) {
        PQfinish(connection);
        connection = nullptr;
    }
}

// Тестирование подключения к БД
bool DatabaseService::testConnection() {
    // Загружаем актуальную конфигурацию перед тестом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    return PQstatus(connection) == CONNECTION_OK;
}

// Настройка структуры базы данных (создание таблиц)
bool DatabaseService::setupDatabase() {
    // Загружаем актуальную конфигурацию перед настройкой БД
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // SQL команды для создания таблиц, если они не существуют
    std::vector<std::string> sqlCommands = {
        // Таблица преподавателей
        "CREATE TABLE IF NOT EXISTS teachers ("
        "teacher_id SERIAL PRIMARY KEY,"
        "last_name VARCHAR(100) NOT NULL,"
        "first_name VARCHAR(100) NOT NULL,"
        "middle_name VARCHAR(100),"
        "experience INTEGER NOT NULL,"
        "specialization VARCHAR(100) NOT NULL,"
        "email VARCHAR(100),"
        "phone_number VARCHAR(20))",
        
        // Таблица студенческих групп
        "CREATE TABLE IF NOT EXISTS student_groups ("
        "group_id SERIAL PRIMARY KEY,"
        "name VARCHAR(50) NOT NULL UNIQUE,"
        "student_count INTEGER DEFAULT 0,"
        "teacher_id INTEGER REFERENCES teachers(teacher_id))",
        
        // Таблица студентов
        "CREATE TABLE IF NOT EXISTS students ("
        "student_code SERIAL PRIMARY KEY,"
        "last_name VARCHAR(100) NOT NULL,"
        "first_name VARCHAR(100) NOT NULL,"
        "middle_name VARCHAR(100),"
        "phone_number VARCHAR(20),"
        "email VARCHAR(100),"
        "group_id INTEGER REFERENCES student_groups(group_id))",
        
        // Таблица портфолио студентов
        "CREATE TABLE IF NOT EXISTS student_portfolio ("
        "portfolio_id SERIAL PRIMARY KEY,"
        "student_code INTEGER REFERENCES students(student_code),"
        "measure_code VARCHAR(100) NOT NULL,"
        "date DATE NOT NULL,"
        "passport_series VARCHAR(10) NOT NULL,"
        "passport_number VARCHAR(10) NOT NULL)"
    };
    
    // Выполняем все SQL команды
    for (const auto& sql : sqlCommands) {
        executeSQL(sql);
    }
    
    std::cout << "Database setup completed!" << std::endl;
    return true;
}

// Выполнение SQL команды
void DatabaseService::executeSQL(const std::string& sql) {
    // Загружаем актуальную конфигурацию перед выполнением SQL
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) return;
    
    PGresult* res = PQexec(connection, sql.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "SQL error: " << PQerrorMessage(connection) << std::endl;
    }
    PQclear(res);
}

// Получение списка преподавателей
std::vector<Teacher> DatabaseService::getTeachers() {
    std::vector<Teacher> teachers;
    
    // Загружаем актуальную конфигурацию перед запросом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) return teachers;
    
    PGresult* res = PQexec(connection, "SELECT * FROM teachers");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return teachers;
    }
    
    // Обрабатываем каждую строку результата
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Teacher teacher;
        teacher.teacherId = std::stoi(PQgetvalue(res, i, 0));
        teacher.lastName = PQgetvalue(res, i, 1);
        teacher.firstName = PQgetvalue(res, i, 2);
        teacher.middleName = PQgetvalue(res, i, 3);
        teacher.experience = std::stoi(PQgetvalue(res, i, 4));
        teacher.specialization = PQgetvalue(res, i, 5);
        teacher.email = PQgetvalue(res, i, 6);
        teacher.phoneNumber = PQgetvalue(res, i, 7);
        
        teachers.push_back(teacher);
    }
    
    PQclear(res);
    return teachers;
}

// Получение списка групп
std::vector<StudentGroup> DatabaseService::getGroups() {
    std::vector<StudentGroup> groups;
    
    // Загружаем актуальную конфигурацию перед запросом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) return groups;
    
    PGresult* res = PQexec(connection, "SELECT * FROM student_groups");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return groups;
    }
    
    // Обрабатываем каждую строку результата
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        StudentGroup group;
        group.groupId = std::stoi(PQgetvalue(res, i, 0));
        group.name = PQgetvalue(res, i, 1);
        group.studentCount = std::stoi(PQgetvalue(res, i, 2));
        group.teacherId = std::stoi(PQgetvalue(res, i, 3));
        
        groups.push_back(group);
    }
    
    PQclear(res);
    return groups;
}

// Добавление студента
bool DatabaseService::addStudent(const Student& student) {
    // Загружаем актуальную конфигурацию перед добавлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Сохраняем числовые значения в отдельные переменные, чтобы избежать проблем с временными объектами
    std::string groupIdStr = std::to_string(student.groupId);
    
    std::string sql = "INSERT INTO students (last_name, first_name, middle_name, phone_number, email, group_id) VALUES ($1, $2, $3, $4, $5, $6)";
    
    // Создаем массив параметров - используем сохраненные переменные вместо временных объектов
    const char* params[6] = {
        student.lastName.c_str(),
        student.firstName.c_str(),
        student.middleName.c_str(),
        student.phoneNumber.c_str(),
        student.email.c_str(),
        groupIdStr.c_str()  // используем сохраненную переменную вместо std::to_string(student.groupId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 6, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

// Добавление преподавателя
bool DatabaseService::addTeacher(const Teacher& teacher) {
    // Загружаем актуальную конфигурацию перед добавлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Сохраняем числовые значения в отдельные переменные
    std::string experienceStr = std::to_string(teacher.experience);
    
    std::string sql = "INSERT INTO teachers (last_name, first_name, middle_name, experience, specialization, email, phone_number) VALUES ($1, $2, $3, $4, $5, $6, $7)";
    
    // Создаем массив параметров
    const char* params[7] = {
        teacher.lastName.c_str(),
        teacher.firstName.c_str(),
        teacher.middleName.c_str(),
        experienceStr.c_str(),
        teacher.specialization.c_str(),
        teacher.email.c_str(),
        teacher.phoneNumber.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 7, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

// Добавление группы
bool DatabaseService::addGroup(const StudentGroup& group) {
    // Загружаем актуальную конфигурацию перед добавлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Сохраняем числовые значения в отдельные переменные
    std::string studentCountStr = std::to_string(group.studentCount);
    std::string teacherIdStr = std::to_string(group.teacherId);
    
    std::string sql = "INSERT INTO student_groups (name, student_count, teacher_id) VALUES ($1, $2, $3)";
    
    // Создаем массив параметров
    const char* params[3] = {
        group.name.c_str(),
        studentCountStr.c_str(),  // используем сохраненную переменную
        teacherIdStr.c_str()      // используем сохраненную переменную
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 3, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

// Добавление портфолио
bool DatabaseService::addPortfolio(const StudentPortfolio& portfolio) {
    // Загружаем актуальную конфигурацию перед добавлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Сохраняем числовые значения в отдельные переменные
    std::string studentCodeStr = std::to_string(portfolio.studentCode);
    
    std::string sql = "INSERT INTO student_portfolio (student_code, measure_code, date, passport_series, passport_number) VALUES ($1, $2, $3, $4, $5)";
    
    // Создаем массив параметров
    const char* params[5] = {
        studentCodeStr.c_str(),  // используем сохраненную переменную
        portfolio.measureCode.c_str(),
        portfolio.date.c_str(),
        portfolio.passportSeries.c_str(),
        portfolio.passportNumber.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 5, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

// Получение списка портфолио
std::vector<StudentPortfolio> DatabaseService::getPortfolios() {
    std::vector<StudentPortfolio> portfolios;
    
    // Загружаем актуальную конфигурацию перед запросом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return portfolios;
    }
    
    PGresult* res = PQexec(connection, "SELECT * FROM student_portfolio");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return portfolios;
    }
    
    // Обрабатываем каждую строку результата
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        StudentPortfolio portfolio;
        portfolio.portfolioId = std::stoi(PQgetvalue(res, i, 0));
        portfolio.studentCode = std::stoi(PQgetvalue(res, i, 1));
        portfolio.measureCode = PQgetvalue(res, i, 2);
        portfolio.date = PQgetvalue(res, i, 3);
        portfolio.passportSeries = PQgetvalue(res, i, 4);
        portfolio.passportNumber = PQgetvalue(res, i, 5);
        
        portfolios.push_back(portfolio);
    }
    
    PQclear(res);
    return portfolios;
}

// Получение списка студентов
std::vector<Student> DatabaseService::getStudents() {
    std::vector<Student> students;

    // Загружаем актуальную конфигурацию перед запросом
    configManager.loadConfig(currentConfig);

    if (!connection && !connect(currentConfig)) {
        return students;
    }

    PGresult* res = PQexec(connection, "SELECT student_code, last_name, first_name, middle_name, phone_number, email, group_id FROM students");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка выполнения запроса: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return students;
    }

    // Обрабатываем каждую строку результата с проверкой на NULL значения
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Student student;

        // Безопасное чтение student_code
        if (!PQgetisnull(res, i, 0)) {
            student.studentCode = std::stoi(PQgetvalue(res, i, 0));
        }

        // Безопасное чтение last_name
        if (!PQgetisnull(res, i, 1)) {
            student.lastName = PQgetvalue(res, i, 1);
        }

        // Безопасное чтение first_name
        if (!PQgetisnull(res, i, 2)) {
            student.firstName = PQgetvalue(res, i, 2);
        }

        // Безопасное чтение middle_name (может быть NULL)
        if (!PQgetisnull(res, i, 3)) {
            student.middleName = PQgetvalue(res, i, 3);
        }

        // Безопасное чтение phone_number (может быть NULL)
        if (!PQgetisnull(res, i, 4)) {
            student.phoneNumber = PQgetvalue(res, i, 4);
        }

        // Безопасное чтение email (может быть NULL)
        if (!PQgetisnull(res, i, 5)) {
            student.email = PQgetvalue(res, i, 5);
        }

        // Безопасное чтение group_id
        if (!PQgetisnull(res, i, 6)) {
            student.groupId = std::stoi(PQgetvalue(res, i, 6));
        }

        students.push_back(student);
    }

    PQclear(res);
    return students;
}