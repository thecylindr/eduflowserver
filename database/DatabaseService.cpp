#include "database/DatabaseService.h"
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
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    return PQstatus(connection) == CONNECTION_OK;
}

bool DatabaseService::setupDatabase() {
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
        "specialization SERIAL UNIQUE,"
        "email VARCHAR(100),"
        "phone_number VARCHAR(20))",

        "CREATE TABLE IF NOT EXISTS specialization_list ("
        "id SERIAL PRIMARY KEY,"
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
        "group_id INTEGER REFERENCES student_groups(group_id),"
        "passport_series VARCHAR(10) NOT NULL,"
        "passport_number VARCHAR(10) NOT NULL)",
        
        "CREATE TABLE IF NOT EXISTS student_portfolio ("
        "portfolio_id SERIAL PRIMARY KEY,"
        "student_code INTEGER REFERENCES students(student_code),"
        "measure_code INTEGER NOT NULL UNIQUE,"
        "date DATE NOT NULL,"
        "decree INTEGER NOT NULL)",
        
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
        "login VARCHAR(24) NOT NULL,"
        "phone_number VARCHAR(20),"
        "password_hash TEXT NOT NULL,"
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
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) return;
    
    PGresult* res = PQexec(connection, sql.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "SQL error: " << PQerrorMessage(connection) << std::endl;
    }
    PQclear(res);
}

bool DatabaseService::addUser(const User& user) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO users (email, login, phone_number, password_hash, last_name, first_name, middle_name) VALUES ($1, $2, $3, $4, $5, $6, $7)";
    const char* params[7] = {
        user.email.c_str(),
        user.login.c_str(),
        user.phoneNumber.c_str(),
        user.passwordHash.c_str(),
        user.lastName.c_str(),
        user.firstName.c_str(),
        user.middleName.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 7, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

User DatabaseService::getUserByEmail(const std::string& email) {
    User user;
    user.userId = 0;
    
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return user;
    }
    
    std::string sql = "SELECT user_id, login, email, phone_number, password_hash, last_name, first_name, middle_name FROM users WHERE email = $1";
    const char* params[1] = { email.c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return user;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        return user;
    }
    
    user.userId = std::stoi(PQgetvalue(res, 0, 0));
    user.login = PQgetvalue(res, 0, 1);
    user.email = PQgetvalue(res, 0, 2);
    user.phoneNumber = PQgetvalue(res, 0, 3);
    user.passwordHash = PQgetvalue(res, 0, 4);
    user.lastName = PQgetvalue(res, 0, 5);
    user.firstName = PQgetvalue(res, 0, 6);
    user.middleName = PQgetvalue(res, 0, 7);
    
    PQclear(res);
    return user;
}

User DatabaseService::getUserByLogin(const std::string& login) {
    User user;
    user.userId = 0;
    
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return user;
    }
    
    std::string sql = "SELECT user_id, login, email, phone_number, password_hash, last_name, first_name, middle_name FROM users WHERE login = $1";
    const char* params[1] = { login.c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return user;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        return user;
    }
    
    user.userId = std::stoi(PQgetvalue(res, 0, 0));
    user.login = PQgetvalue(res, 0, 1);
    user.email = PQgetvalue(res, 0, 2);
    user.phoneNumber = PQgetvalue(res, 0, 3);
    user.passwordHash = PQgetvalue(res, 0, 4);
    user.lastName = PQgetvalue(res, 0, 5);
    user.firstName = PQgetvalue(res, 0, 6);
    user.middleName = PQgetvalue(res, 0, 7);
    
    PQclear(res);
    return user;
}

User DatabaseService::getUserByPhoneNumber(const std::string& phoneNumber) {
    User user;
    user.userId = 0;
    
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return user;
    }
    
    std::string sql = "SELECT user_id, login, email, phone_number, password_hash, last_name, first_name, middle_name FROM users WHERE phone_number = $1";
    const char* params[1] = { phoneNumber.c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return user;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        return user;
    }
    
    user.userId = std::stoi(PQgetvalue(res, 0, 0));
    user.login = PQgetvalue(res, 0, 1);
    user.email = PQgetvalue(res, 0, 2);
    user.phoneNumber = PQgetvalue(res, 0, 3);
    user.passwordHash = PQgetvalue(res, 0, 4);
    user.lastName = PQgetvalue(res, 0, 5);
    user.firstName = PQgetvalue(res, 0, 6);
    user.middleName = PQgetvalue(res, 0, 7);
    
    PQclear(res);
    return user;
}

bool DatabaseService::updateUser(const User& user) {
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

User DatabaseService::getUserById(int userId) {
    User user;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return user;
    }
    
    std::string sql = "SELECT user_id, email, login, phone_number, password_hash, last_name, first_name, middle_name FROM users WHERE user_id = $1";
    const char* params[1] = { std::to_string(userId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return user;
    }
    
    if (PQntuples(res) == 0) {
        PQclear(res);
        return user;
    }
    
    user.userId = std::stoi(PQgetvalue(res, 0, 0));
    user.email = PQgetvalue(res, 0, 1);
    user.login = PQgetvalue(res, 0, 2);
    user.phoneNumber = PQgetvalue(res, 0, 3);
    user.passwordHash = PQgetvalue(res, 0, 4);
    user.lastName = PQgetvalue(res, 0, 5);
    user.firstName = PQgetvalue(res, 0, 6);
    user.middleName = PQgetvalue(res, 0, 7);
    
    PQclear(res);
    return user;
}

std::vector<StudentGroup> DatabaseService::getGroups() {
    std::vector<StudentGroup> groups;
    
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
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO students (last_name, first_name, middle_name, phone_number, email, group_id, passport_series, passport_number) VALUES ($1, $2, $3, $4, $5, $6, $7, $8)";
    const char* params[8] = {
        student.lastName.c_str(),
        student.firstName.c_str(),
        student.middleName.c_str(),
        student.phoneNumber.c_str(),
        student.email.c_str(),
        std::to_string(student.groupId).c_str(),
        student.passportSeries.c_str(),
        student.passportNumber.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 8, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::addTeacherSpecialization(int teacherId, int specializationCode) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO teacher_specializations (teacher_id, specialization_code) VALUES ($1, $2)";
    const char* params[2] = {
        std::to_string(teacherId).c_str(),
        std::to_string(specializationCode).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 2, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::addTeacher(const Teacher& teacher) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // specialization теперь SERIAL - не передаём его, БД сама сгенерирует
    std::string sql = "INSERT INTO teachers (last_name, first_name, middle_name, experience, email, phone_number) VALUES ($1, $2, $3, $4, $5, $6) RETURNING teacher_id, specialization";
    const char* params[6] = {
        teacher.lastName.c_str(),
        teacher.firstName.c_str(),
        teacher.middleName.c_str(),
        std::to_string(teacher.experience).c_str(),
        teacher.email.c_str(),
        teacher.phoneNumber.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 6, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Database error in addTeacher: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return false;
    }
    
    // Получаем ID преподавателя и сгенерированный код специализации
    int teacherId = std::stoi(PQgetvalue(res, 0, 0));
    int specializationCode = std::stoi(PQgetvalue(res, 0, 1));
    PQclear(res);
    
    std::cout << "✅ Teacher added with ID: " << teacherId << ", specialization code: " << specializationCode << std::endl;
    
    // Теперь добавляем специализации в specialization_list
    if (!teacher.specializations.empty()) {
        for (const auto& spec : teacher.specializations) {
            std::string specSql = "INSERT INTO specialization_list (specialization, name) VALUES ($1, $2)";
            const char* specParams[2] = {
                std::to_string(specializationCode).c_str(),  // используем сгенерированный код
                spec.name.c_str()
            };
            
            PGresult* specRes = PQexecParams(connection, specSql.c_str(), 2, NULL, specParams, NULL, NULL, 0);
            if (PQresultStatus(specRes) != PGRES_COMMAND_OK) {
                std::cerr << "Warning: Failed to add specialization: " << PQerrorMessage(connection) << std::endl;
            } else {
                std::cout << "✅ Added specialization: " << spec.name << " with code: " << specializationCode << std::endl;
            }
            PQclear(specRes);
        }
    }
    
    return true;
}

std::vector<Teacher> DatabaseService::getTeachers() {
    std::vector<Teacher> teachers;
    
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) return teachers;
    
    // Используем JOIN чтобы получить специализации
    std::string sql = "SELECT t.teacher_id, t.last_name, t.first_name, t.middle_name, t.experience, t.email, t.phone_number, "
                     "STRING_AGG(sl.name, ', ') as specializations "
                     "FROM teachers t "
                     "LEFT JOIN specialization_list sl ON t.specialization = sl.specialization "
                     "GROUP BY t.teacher_id, t.last_name, t.first_name, t.middle_name, t.experience, t.email, t.phone_number";
    
    PGresult* res = PQexec(connection, sql.c_str());
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
        teacher.email = PQgetvalue(res, i, 5);
        teacher.phoneNumber = PQgetvalue(res, i, 6);
        
        // Получаем специализации как строку
        if (!PQgetisnull(res, i, 7)) {
            teacher.specialization = PQgetvalue(res, i, 7);
        } else {
            teacher.specialization = "";
        }
        
        teachers.push_back(teacher);
    }
    
    PQclear(res);
    return teachers;
}

bool DatabaseService::addGroup(const StudentGroup& group) {
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
    configManager.loadConfig(currentConfig);

    if (!connection && !connect(currentConfig)) {
        return students;
    }

    PGresult* res = PQexec(connection, 
        "SELECT student_code, last_name, first_name, middle_name, phone_number, email, group_id, passport_series, passport_number FROM students");

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка выполнения запроса: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return students;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Student student;

        if (!PQgetisnull(res, i, 0)) {
            student.studentCode = std::stoi(PQgetvalue(res, i, 0));
        }

        if (!PQgetisnull(res, i, 1)) {
            student.lastName = PQgetvalue(res, i, 1);
        }

        if (!PQgetisnull(res, i, 2)) {
            student.firstName = PQgetvalue(res, i, 2);
        }

        if (!PQgetisnull(res, i, 3)) {
            student.middleName = PQgetvalue(res, i, 3);
        }

        if (!PQgetisnull(res, i, 4)) {
            student.phoneNumber = PQgetvalue(res, i, 4);
        }

        if (!PQgetisnull(res, i, 5)) {
            student.email = PQgetvalue(res, i, 5);
        }

        if (!PQgetisnull(res, i, 6)) {
            student.groupId = std::stoi(PQgetvalue(res, i, 6));
        }

        if (!PQgetisnull(res, i, 7)) {
            student.passportSeries = PQgetvalue(res, i, 7);
        }
        if (!PQgetisnull(res, i, 8)) {
            student.passportNumber = PQgetvalue(res, i, 8);
        }

        students.push_back(student);
    }

    PQclear(res);
    return students;
}

bool DatabaseService::updateTeacher(const Teacher& teacher) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "UPDATE teachers SET last_name = $1, first_name = $2, middle_name = $3, experience = $4, email = $5, phone_number = $6 WHERE teacher_id = $7";
    const char* params[7] = {
        teacher.lastName.c_str(),
        teacher.firstName.c_str(),
        teacher.middleName.c_str(),
        std::to_string(teacher.experience).c_str(),
        teacher.email.c_str(),
        teacher.phoneNumber.c_str(),
        std::to_string(teacher.teacherId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 7, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::deleteTeacher(int teacherId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Сначала удаляем специализации преподавателя
    std::string deleteSpecsSql = "DELETE FROM specialization_list WHERE specialization IN (SELECT specialization FROM teachers WHERE teacher_id = $1)";
    const char* deleteSpecsParams[1] = { std::to_string(teacherId).c_str() };
    
    PGresult* deleteSpecsRes = PQexecParams(connection, deleteSpecsSql.c_str(), 1, NULL, deleteSpecsParams, NULL, NULL, 0);
    PQclear(deleteSpecsRes);
    
    // Затем удаляем преподавателя
    std::string sql = "DELETE FROM teachers WHERE teacher_id = $1";
    const char* params[1] = { std::to_string(teacherId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::updateStudent(const Student& student) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "UPDATE students SET last_name = $1, first_name = $2, middle_name = $3, phone_number = $4, email = $5, group_id = $6, passport_series = $7, passport_number = $8 WHERE student_code = $9";
    const char* params[9] = {
        student.lastName.c_str(),
        student.firstName.c_str(),
        student.middleName.c_str(),
        student.phoneNumber.c_str(),
        student.email.c_str(),
        std::to_string(student.groupId).c_str(),
        student.passportSeries.c_str(),
        student.passportNumber.c_str(),
        std::to_string(student.studentCode).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 9, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::deleteStudent(int studentCode) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "DELETE FROM students WHERE student_code = $1";
    const char* params[1] = { std::to_string(studentCode).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

Teacher DatabaseService::getTeacherById(int teacherId) {
    Teacher teacher;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return teacher;
    }
    
    std::string sql = "SELECT teacher_id, last_name, first_name, middle_name, experience, specialization, email, phone_number FROM teachers WHERE teacher_id = $1";
    const char* params[1] = { std::to_string(teacherId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return teacher;
    }
    
    teacher.teacherId = std::stoi(PQgetvalue(res, 0, 0));
    teacher.lastName = PQgetvalue(res, 0, 1);
    teacher.firstName = PQgetvalue(res, 0, 2);
    teacher.middleName = PQgetvalue(res, 0, 3);
    teacher.experience = std::stoi(PQgetvalue(res, 0, 4));
    
    // ДОБАВЛЯЕМ КОД СПЕЦИАЛИЗАЦИИ
    if (!PQgetisnull(res, 0, 5)) {
        teacher.specializationCode = std::stoi(PQgetvalue(res, 0, 5));
    } else {
        teacher.specializationCode = 0;
    }
    
    teacher.email = PQgetvalue(res, 0, 6);
    teacher.phoneNumber = PQgetvalue(res, 0, 7);
    
    PQclear(res);
    return teacher;
}

bool DatabaseService::removeAllTeacherSpecializations(int teacherId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Получаем код специализации преподавателя
    Teacher teacher = getTeacherById(teacherId);
    if (teacher.teacherId == 0) {
        return false;
    }
    
    std::string sql = "DELETE FROM specialization_list WHERE specialization = $1";
    const char* params[1] = { std::to_string(teacher.specializationCode).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

Student DatabaseService::getStudentById(int studentId) {
    Student student;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return student;
    }
    
    std::string sql = "SELECT student_code, last_name, first_name, middle_name, phone_number, email, group_id, passport_series, passport_number FROM students WHERE student_code = $1";
    const char* params[1] = { std::to_string(studentId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return student;
    }
    
    student.studentCode = std::stoi(PQgetvalue(res, 0, 0));
    student.lastName = PQgetvalue(res, 0, 1);
    student.firstName = PQgetvalue(res, 0, 2);
    student.middleName = PQgetvalue(res, 0, 3);
    student.phoneNumber = PQgetvalue(res, 0, 4);
    student.email = PQgetvalue(res, 0, 5);
    student.groupId = std::stoi(PQgetvalue(res, 0, 6));
    student.passportSeries = PQgetvalue(res, 0, 7);
    student.passportNumber = PQgetvalue(res, 0, 8);
    
    PQclear(res);
    return student;
}

StudentGroup DatabaseService::getGroupById(int groupId) {
    StudentGroup group;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return group;
    }
    
    std::string sql = "SELECT group_id, name, student_count, teacher_id FROM student_groups WHERE group_id = $1";
    const char* params[1] = { std::to_string(groupId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return group;
    }
    
    group.groupId = std::stoi(PQgetvalue(res, 0, 0));
    group.name = PQgetvalue(res, 0, 1);
    group.studentCount = std::stoi(PQgetvalue(res, 0, 2));
    group.teacherId = std::stoi(PQgetvalue(res, 0, 3));
    
    PQclear(res);
    return group;
}

bool DatabaseService::updateGroup(const StudentGroup& group) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "UPDATE student_groups SET name = $1, student_count = $2, teacher_id = $3 WHERE group_id = $4";
    const char* params[4] = {
        group.name.c_str(),
        std::to_string(group.studentCount).c_str(),
        std::to_string(group.teacherId).c_str(),
        std::to_string(group.groupId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 4, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::deleteGroup(int groupId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "DELETE FROM student_groups WHERE group_id = $1";
    const char* params[1] = { std::to_string(groupId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

std::vector<Event> DatabaseService::getEvents() {
    std::vector<Event> events;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return events;
    }
    
    PGresult* res = PQexec(connection, "SELECT event_id, event_category, event_type, start_date, end_date, location, lore FROM event");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return events;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Event event;
        event.eventId = std::stoi(PQgetvalue(res, i, 0));
        event.eventCategory = PQgetvalue(res, i, 1);
        event.eventType = PQgetvalue(res, i, 2);
        event.startDate = PQgetvalue(res, i, 3);
        event.endDate = PQgetvalue(res, i, 4);
        event.location = PQgetvalue(res, i, 5);
        event.lore = PQgetvalue(res, i, 6);
        
        events.push_back(event);
    }
    
    PQclear(res);
    return events;
}

bool DatabaseService::addEvent(const Event& event) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO event (event_id, event_category, event_type, start_date, end_date, location, lore) VALUES ($1, $2, $3, $4, $5, $6, $7)";
    const char* params[7] = {
        std::to_string(event.eventId).c_str(),
        event.eventCategory.c_str(),
        event.eventType.c_str(),
        event.startDate.c_str(),
        event.endDate.c_str(),
        event.location.c_str(),
        event.lore.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 7, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::addSpecialization(const Specialization& specialization) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO specialization_list (specialization, name) VALUES ($1, $2)";
    const char* params[2] = {
        std::to_string(specialization.specializationCode).c_str(),
        specialization.name.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 2, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

bool DatabaseService::deleteSpecialization(int specializationCode) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "DELETE FROM specialization_list WHERE specialization = $1";
    const char* params[1] = { std::to_string(specializationCode).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

std::vector<Specialization> DatabaseService::getSpecializations() {
    std::vector<Specialization> specializations;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return specializations;
    }
    
    PGresult* res = PQexec(connection, "SELECT specialization, name FROM specialization_list ORDER BY name");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return specializations;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Specialization spec;
        spec.specializationCode = std::stoi(PQgetvalue(res, i, 0));
        spec.name = PQgetvalue(res, i, 1);
        specializations.push_back(spec);
    }
    
    PQclear(res);
    return specializations;
}

std::vector<Specialization> DatabaseService::getTeacherSpecializations(int teacherId) {
    std::vector<Specialization> specializations;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return specializations;
    }
    
    // Получаем специализации преподавателя из таблицы specialization_list
    std::string sql = "SELECT specialization, name FROM specialization_list WHERE specialization IN (SELECT specialization FROM teachers WHERE teacher_id = $1)";
    const char* params[1] = { std::to_string(teacherId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        for (int i = 0; i < rows; i++) {
            Specialization spec;
            spec.specializationCode = std::stoi(PQgetvalue(res, i, 0));
            spec.name = PQgetvalue(res, i, 1);
            specializations.push_back(spec);
        }
    }
    PQclear(res);
    
    return specializations;
}

bool DatabaseService::removeTeacherSpecialization(int teacherId, int specializationCode) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Удаляем специализацию из specialization_list
    std::string sql = "DELETE FROM specialization_list WHERE specialization = $1 AND specialization IN (SELECT specialization FROM teachers WHERE teacher_id = $2)";
    const char* params[2] = {
        std::to_string(specializationCode).c_str(),
        std::to_string(teacherId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 2, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}