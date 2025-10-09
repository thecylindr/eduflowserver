#include "DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>

DatabaseService::DatabaseService() : connection(nullptr) {
    configManager.loadConfig(currentConfig);
}

DatabaseService::~DatabaseService() {
    disconnect();
}

bool DatabaseService::connect(const DatabaseConfig& config) {
    if (connection) {
        disconnect();
    }
    
    currentConfig = config;
    std::string connStr = "host=" + config.host + 
                         " port=" + std::to_string(config.port) + 
                         " dbname=" + config.database + 
                         " user=" + config.username + 
                         " password=" + config.password;
    
    connection = PQconnectdb(connStr.c_str());
    
    if (PQstatus(connection) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(connection) << std::endl;
        PQfinish(connection);
        connection = nullptr;
        return false;
    }
    
    return true;
}

void DatabaseService::disconnect() {
    if (connection) {
        PQfinish(connection);
        connection = nullptr;
    }
}

bool DatabaseService::testConnection() {
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед тестом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    return PQstatus(connection) == CONNECTION_OK;
}

bool DatabaseService::setupDatabase() {
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед настройкой БД
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::vector<std::string> sqlCommands = {
        "CREATE TABLE IF NOT EXISTS teachers ("
        "teacher_id SERIAL PRIMARY KEY,"
        "last_name VARCHAR(100) NOT NULL,"
        "first_name VARCHAR(100) NOT NULL,"
        "middle_name VARCHAR(100),"
        "experience INTEGER NOT NULL,"
        "specialization INTEGER NOT NULL UNIQUE,"
        "email VARCHAR(100),"
        "phone_number VARCHAR(20))",

        "CREATE TABLE IF NOT EXISTS specialization_list ("
        "specialization INTEGER REFERENCES teachers(specialization),"
        "name VARCHAR(80) NOT NULL)",
        
        "CREATE TABLE IF NOT EXISTS student_groups ("
        "group_id SERIAL PRIMARY KEY,"
        "name VARCHAR(50) NOT NULL UNIQUE,"
        "student_count INTEGER DEFAULT 0,"
        "teacher_id INTEGER REFERENCES teachers(teacher_id))",
        
        "CREATE TABLE IF NOT EXISTS students ("
        "student_code SERIAL PRIMARY KEY,"
        "last_name VARCHAR(100) NOT NULL,"
        "first_name VARCHAR(100) NOT NULL,"
        "middle_name VARCHAR(100),"
        "phone_number VARCHAR(20),"
        "email VARCHAR(100),"
        "group_id INTEGER REFERENCES student_groups(group_id))",
        
        "CREATE TABLE IF NOT EXISTS student_portfolio ("
        "portfolio_id SERIAL PRIMARY KEY,"
        "student_code INTEGER REFERENCES students(student_code),"
        "measure_code INTEGER NOT NULL UNIQUE,"
        "date DATE NOT NULL,"
        "passport_series VARCHAR(10) NOT NULL,"
        "passport_number VARCHAR(10) NOT NULL)",

        "CREATE TABLE IF NOT EXISTS event ("
        "event_id INTEGER REFERENCES student_portfolio(measure_code),"
        "event_category VARCHAR(48) NOT NULL UNIQUE,"
        "event_type VARCHAR(50) NOT NULL,"
        "start_date TIMESTAMP WITHOUT TIME ZONE NOT NULL,"
        "end_date TIMESTAMP WITHOUT TIME ZONE NOT NULL,"
        "location VARCHAR(48),"
        "lore TEXT)",

        "CREATE TABLE IF NOT EXISTS event_category ("
        "event_type VARCHAR(48) REFERENCES event(event_category),"
        "category VARCHAR(48))",

        "CREATE TABLE IF NOT EXISTS users ("
        "user_id SERIAL PRIMARY KEY,"
        "email VARCHAR(100) UNIQUE NOT NULL,"
        "phone_number VARCHAR(20),"
        "password_hash VARCHAR(255) NOT NULL,"
        "last_name VARCHAR(100) NOT NULL,"
        "first_name VARCHAR(100) NOT NULL,"
        "middle_name VARCHAR(100))"
    };
    
    for (const auto& sql : sqlCommands) {
        executeSQL(sql);
    }
    
    std::cout << "Database setup completed!" << std::endl;
    return true;
}

void DatabaseService::executeSQL(const std::string& sql) {
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед выполнением SQL
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) return;
    
    PGresult* res = PQexec(connection, sql.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "SQL error: " << PQerrorMessage(connection) << std::endl;
    }
    PQclear(res);
}

// User management methods
bool DatabaseService::addUser(const User& user) {
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед добавлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO users (email, phone_number, password_hash, last_name, first_name, middle_name) VALUES ($1, $2, $3, $4, $5, $6)";
    const char* params[6] = {
        user.email.c_str(),
        user.phoneNumber.c_str(),
        user.passwordHash.c_str(),
        user.lastName.c_str(),
        user.firstName.c_str(),
        user.middleName.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 6, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::updateUser(const User& user) {
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед обновлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "UPDATE users SET email = $1, phone_number = $2, password_hash = $3, last_name = $4, first_name = $5, middle_name = $6 WHERE user_id = $7";
    const char* params[7] = {
        user.email.c_str(),
        user.phoneNumber.c_str(),
        user.passwordHash.c_str(),
        user.lastName.c_str(),
        user.firstName.c_str(),
        user.middleName.c_str(),
        std::to_string(user.userId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 7, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

User DatabaseService::getUserByEmail(const std::string& email) {
    User user;
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед запросом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return user;
    }
    
    std::string sql = "SELECT user_id, email, phone_number, password_hash, last_name, first_name, middle_name FROM users WHERE email = $1";
    const char* params[1] = { email.c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return user;
    }
    
    user.userId = std::stoi(PQgetvalue(res, 0, 0));
    user.email = PQgetvalue(res, 0, 1);
    user.phoneNumber = PQgetvalue(res, 0, 2);
    user.passwordHash = PQgetvalue(res, 0, 3);
    user.lastName = PQgetvalue(res, 0, 4);
    user.firstName = PQgetvalue(res, 0, 5);
    user.middleName = PQgetvalue(res, 0, 6);
    
    PQclear(res);
    return user;
}

User DatabaseService::getUserById(int userId) {
    User user;
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед запросом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return user;
    }
    
    std::string sql = "SELECT user_id, email, phone_number, password_hash, last_name, first_name, middle_name FROM users WHERE user_id = $1";
    const char* params[1] = { std::to_string(userId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return user;
    }
    
    user.userId = std::stoi(PQgetvalue(res, 0, 0));
    user.email = PQgetvalue(res, 0, 1);
    user.phoneNumber = PQgetvalue(res, 0, 2);
    user.passwordHash = PQgetvalue(res, 0, 3);
    user.lastName = PQgetvalue(res, 0, 4);
    user.firstName = PQgetvalue(res, 0, 5);
    user.middleName = PQgetvalue(res, 0, 6);
    
    PQclear(res);
    return user;
}

// Existing methods remain the same...
std::vector<Teacher> DatabaseService::getTeachers() {
    std::vector<Teacher> teachers;
    
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед запросом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) return teachers;
    
    PGresult* res = PQexec(connection, "SELECT * FROM teachers");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return teachers;
    }
    
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

std::vector<StudentGroup> DatabaseService::getGroups() {
    std::vector<StudentGroup> groups;
    
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед запросом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) return groups;
    
    PGresult* res = PQexec(connection, "SELECT * FROM student_groups");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return groups;
    }
    
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

bool DatabaseService::addStudent(const Student& student) {
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед добавлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO students (last_name, first_name, middle_name, phone_number, email, group_id) VALUES ($1, $2, $3, $4, $5, $6)";
    const char* params[6] = {
        student.lastName.c_str(),
        student.firstName.c_str(),
        student.middleName.c_str(),
        student.phoneNumber.c_str(),
        student.email.c_str(),
        std::to_string(student.groupId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 6, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::addTeacher(const Teacher& teacher) {
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед добавлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO teachers (last_name, first_name, middle_name, experience, specialization, email, phone_number) VALUES ($1, $2, $3, $4, $5, $6, $7)";
    const char* params[7] = {
        teacher.lastName.c_str(),
        teacher.firstName.c_str(),
        teacher.middleName.c_str(),
        std::to_string(teacher.experience).c_str(),
        teacher.specialization.c_str(),
        teacher.email.c_str(),
        teacher.phoneNumber.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 7, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::addGroup(const StudentGroup& group) {
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед добавлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO student_groups (name, student_count, teacher_id) VALUES ($1, $2, $3)";
    const char* params[3] = {
        group.name.c_str(),
        std::to_string(group.studentCount).c_str(),
        std::to_string(group.teacherId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 3, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::addPortfolio(const StudentPortfolio& portfolio) {
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед добавлением
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO student_portfolio (student_code, measure_code, date, passport_series, passport_number) VALUES ($1, $2, $3, $4, $5)";
    const char* params[5] = {
        std::to_string(portfolio.studentCode).c_str(),
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

std::vector<StudentPortfolio> DatabaseService::getPortfolios() {
    std::vector<StudentPortfolio> portfolios;
    
    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед запросом
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return portfolios;
    }
    
    PGresult* res = PQexec(connection, "SELECT * FROM student_portfolio");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return portfolios;
    }
    
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

std::vector<Student> DatabaseService::getStudents() {
    std::vector<Student> students;

    // ПЕРЕЗАГРУЖАЕМ конфигурацию перед запросом
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