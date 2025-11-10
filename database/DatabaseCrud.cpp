#include "database/DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>

// User management
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

// Teacher management
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
    const char* params[1] = { teacher.specialization.c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

// Student management
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

// Student management - с автоматическим обновлением счетчика студентов
bool DatabaseService::addStudent(const Student& student) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Начинаем транзакцию
    PGresult* beginRes = PQexec(connection, "BEGIN");
    if (PQresultStatus(beginRes) != PGRES_COMMAND_OK) {
        PQclear(beginRes);
        return false;
    }
    PQclear(beginRes);
    
    bool success = true;
    
    // 1. Добавляем студента
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
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        success = false;
        std::cerr << "Ошибка добавления студента: " << PQerrorMessage(connection) << std::endl;
    }
    PQclear(res);
    
    // 2. Обновляем счетчик студентов в группе
    if (success && student.groupId > 0) {
        std::string updateSql = "UPDATE student_groups SET student_count = student_count + 1 WHERE group_id = $1";
        const char* updateParams[1] = { std::to_string(student.groupId).c_str() };
        
        PGresult* updateRes = PQexecParams(connection, updateSql.c_str(), 1, NULL, updateParams, NULL, NULL, 0);
        if (PQresultStatus(updateRes) != PGRES_COMMAND_OK) {
            success = false;
            std::cerr << "Ошибка обновления счетчика группы: " << PQerrorMessage(connection) << std::endl;
        }
        PQclear(updateRes);
    }
    
    // Завершаем транзакцию
    if (success) {
        PGresult* commitRes = PQexec(connection, "COMMIT");
        PQclear(commitRes);
        std::cout << "✅ Студент добавлен, счетчик группы обновлен" << std::endl;
    } else {
        PGresult* rollbackRes = PQexec(connection, "ROLLBACK");
        PQclear(rollbackRes);
        std::cerr << "❌ Ошибка при добавлении студента, транзакция откатана" << std::endl;
    }
    
    return success;
}

bool DatabaseService::updateStudent(const Student& student) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Получаем текущие данные студента для определения старой группы
    Student oldStudent = getStudentById(student.studentCode);
    if (oldStudent.studentCode == 0) {
        return false;
    }
    
    // Начинаем транзакцию
    PGresult* beginRes = PQexec(connection, "BEGIN");
    if (PQresultStatus(beginRes) != PGRES_COMMAND_OK) {
        PQclear(beginRes);
        return false;
    }
    PQclear(beginRes);
    
    bool success = true;
    
    // 1. Обновляем данные студента
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
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        success = false;
        std::cerr << "Ошибка обновления студента: " << PQerrorMessage(connection) << std::endl;
    }
    PQclear(res);
    
    // 2. Обновляем счетчики студентов в группах (если группа изменилась)
    if (success && oldStudent.groupId != student.groupId) {
        // Уменьшаем счетчик в старой группе (если была группа)
        if (oldStudent.groupId > 0) {
            std::string decreaseSql = "UPDATE student_groups SET student_count = student_count - 1 WHERE group_id = $1";
            const char* decreaseParams[1] = { std::to_string(oldStudent.groupId).c_str() };
            
            PGresult* decreaseRes = PQexecParams(connection, decreaseSql.c_str(), 1, NULL, decreaseParams, NULL, NULL, 0);
            if (PQresultStatus(decreaseRes) != PGRES_COMMAND_OK) {
                success = false;
                std::cerr << "Ошибка уменьшения счетчика старой группы: " << PQerrorMessage(connection) << std::endl;
            }
            PQclear(decreaseRes);
        }
        
        // Увеличиваем счетчик в новой группе (если указана новая группа)
        if (success && student.groupId > 0) {
            std::string increaseSql = "UPDATE student_groups SET student_count = student_count + 1 WHERE group_id = $1";
            const char* increaseParams[1] = { std::to_string(student.groupId).c_str() };
            
            PGresult* increaseRes = PQexecParams(connection, increaseSql.c_str(), 1, NULL, increaseParams, NULL, NULL, 0);
            if (PQresultStatus(increaseRes) != PGRES_COMMAND_OK) {
                success = false;
                std::cerr << "Ошибка увеличения счетчика новой группы: " << PQerrorMessage(connection) << std::endl;
            }
            PQclear(increaseRes);
        }
        
        if (success) {
            std::cout << "🔄 Счетчики групп обновлены: старая группа " << oldStudent.groupId 
                      << " -> новая группа " << student.groupId << std::endl;
        }
    }
    
    // Завершаем транзакцию
    if (success) {
        PGresult* commitRes = PQexec(connection, "COMMIT");
        PQclear(commitRes);
        std::cout << "✅ Студент обновлен" << std::endl;
    } else {
        PGresult* rollbackRes = PQexec(connection, "ROLLBACK");
        PQclear(rollbackRes);
        std::cerr << "❌ Ошибка при обновлении студента, транзакция откатана" << std::endl;
    }
    
    return success;
}

bool DatabaseService::deleteStudent(int studentCode) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Получаем данные студента для определения группы
    Student student = getStudentById(studentCode);
    if (student.studentCode == 0) {
        return false;
    }
    
    // Начинаем транзакцию
    PGresult* beginRes = PQexec(connection, "BEGIN");
    if (PQresultStatus(beginRes) != PGRES_COMMAND_OK) {
        PQclear(beginRes);
        return false;
    }
    PQclear(beginRes);
    
    bool success = true;
    
    // 1. Удаляем студента
    std::string sql = "DELETE FROM students WHERE student_code = $1";
    const char* params[1] = { std::to_string(studentCode).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        success = false;
        std::cerr << "Ошибка удаления студента: " << PQerrorMessage(connection) << std::endl;
    }
    PQclear(res);
    
    // 2. Обновляем счетчик студентов в группе (если студент был в группе)
    if (success && student.groupId > 0) {
        std::string updateSql = "UPDATE student_groups SET student_count = student_count - 1 WHERE group_id = $1";
        const char* updateParams[1] = { std::to_string(student.groupId).c_str() };
        
        PGresult* updateRes = PQexecParams(connection, updateSql.c_str(), 1, NULL, updateParams, NULL, NULL, 0);
        if (PQresultStatus(updateRes) != PGRES_COMMAND_OK) {
            success = false;
            std::cerr << "Ошибка обновления счетчика группы: " << PQerrorMessage(connection) << std::endl;
        }
        PQclear(updateRes);
        
        if (success) {
            std::cout << "🔽 Счетчик группы " << student.groupId << " уменьшен на 1" << std::endl;
        }
    }
    
    // Завершаем транзакцию
    if (success) {
        PGresult* commitRes = PQexec(connection, "COMMIT");
        PQclear(commitRes);
        std::cout << "✅ Студент удален" << std::endl;
    } else {
        PGresult* rollbackRes = PQexec(connection, "ROLLBACK");
        PQclear(rollbackRes);
        std::cerr << "❌ Ошибка при удалении студента, транзакция откатана" << std::endl;
    }
    
    return success;
}

// Вспомогательный метод для получения количества студентов в группе
int DatabaseService::getStudentCountInGroup(int groupId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return 0;
    }
    
    std::string sql = "SELECT COUNT(*) FROM students WHERE group_id = $1";
    const char* params[1] = { std::to_string(groupId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return 0;
    }
    
    int count = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    
    return count;
}

// Метод для синхронизации счетчиков студентов во всех группах
bool DatabaseService::syncStudentCounts() {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Начинаем транзакцию
    PGresult* beginRes = PQexec(connection, "BEGIN");
    if (PQresultStatus(beginRes) != PGRES_COMMAND_OK) {
        PQclear(beginRes);
        return false;
    }
    PQclear(beginRes);
    
    bool success = true;
    
    // Получаем все группы
    std::vector<StudentGroup> groups = getGroups();
    
    for (const auto& group : groups) {
        // Получаем актуальное количество студентов в группе
        int actualCount = getStudentCountInGroup(group.groupId);
        
        // Обновляем счетчик в таблице групп
        std::string sql = "UPDATE student_groups SET student_count = $1 WHERE group_id = $2";
        const char* params[2] = {
            std::to_string(actualCount).c_str(),
            std::to_string(group.groupId).c_str()
        };
        
        PGresult* res = PQexecParams(connection, sql.c_str(), 2, NULL, params, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            success = false;
            std::cerr << "Ошибка синхронизации счетчика для группы " << group.groupId 
                      << ": " << PQerrorMessage(connection) << std::endl;
        }
        PQclear(res);
        
        if (success) {
            std::cout << "🔄 Группа " << group.groupId << ": " << group.studentCount 
                      << " -> " << actualCount << " студентов" << std::endl;
        }
    }
    
    // Завершаем транзакцию
    if (success) {
        PGresult* commitRes = PQexec(connection, "COMMIT");
        PQclear(commitRes);
        std::cout << "✅ Синхронизация счетчиков студентов завершена" << std::endl;
    } else {
        PGresult* rollbackRes = PQexec(connection, "ROLLBACK");
        PQclear(rollbackRes);
        std::cerr << "❌ Ошибка при синхронизации счетчиков, транзакция откатана" << std::endl;
    }
    
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

// Group management
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

// Specializations management
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

// Teacher specializations management
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


// Portfolio management - полный CRUD
std::vector<StudentPortfolio> DatabaseService::getPortfolios() {
    std::vector<StudentPortfolio> portfolios;
    
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return portfolios;
    }
    
    // Обновленный запрос с JOIN для получения имени студента
    std::string sql = R"(
        SELECT sp.portfolio_id, sp.student_code, sp.measure_code, sp.date, 
               sp.passport_series, sp.passport_number, sp.description, sp.file_path,
               s.last_name, s.first_name, s.middle_name
        FROM student_portfolio sp
        LEFT JOIN students s ON sp.student_code = s.student_code
        ORDER BY sp.date DESC
    )";
    
    PGresult* res = PQexec(connection, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка выполнения запроса портфолио: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return portfolios;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        StudentPortfolio portfolio;
        portfolio.portfolioId = std::stoi(PQgetvalue(res, i, 0));
        portfolio.studentCode = std::stoi(PQgetvalue(res, i, 1));
        portfolio.measureCode = std::stoi(PQgetvalue(res, i, 2)); // measure_code теперь integer
        portfolio.date = PQgetvalue(res, i, 3);
        portfolio.passportSeries = PQgetvalue(res, i, 4);
        portfolio.passportNumber = PQgetvalue(res, i, 5);
        portfolio.description = PQgetvalue(res, i, 6);
        portfolio.filePath = PQgetvalue(res, i, 7);
        
        // Формируем полное имя студента
        std::string lastName = PQgetvalue(res, i, 8);
        std::string firstName = PQgetvalue(res, i, 9);
        std::string middleName = PQgetvalue(res, i, 10);
        portfolio.studentName = lastName + " " + firstName + " " + middleName;
        
        portfolios.push_back(portfolio);
    }
    
    PQclear(res);
    return portfolios;
}

bool DatabaseService::addPortfolio(const StudentPortfolio& portfolio) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // measure_code не передаем - генерируется автоматически
    std::string sql = R"(
        INSERT INTO student_portfolio 
        (student_code, date, passport_series, passport_number, description, file_path) 
        VALUES ($1, $2, $3, $4, $5, $6)
        RETURNING measure_code
    )";
    
    const char* params[6] = {
        std::to_string(portfolio.studentCode).c_str(),
        portfolio.date.c_str(),
        portfolio.passportSeries.c_str(),
        portfolio.passportNumber.c_str(),
        portfolio.description.c_str(),
        portfolio.filePath.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 6, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_TUPLES_OK);
    
    if (success) {
        // Получаем сгенерированный measure_code
        int generatedMeasureCode = std::stoi(PQgetvalue(res, 0, 0));
        std::cout << "✅ Портфолио добавлено с measure_code: " << generatedMeasureCode << std::endl;
    } else {
        std::cerr << "Ошибка добавления портфолио: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

bool DatabaseService::updatePortfolio(const StudentPortfolio& portfolio) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = R"(
        UPDATE student_portfolio 
        SET student_code = $1, date = $2, passport_series = $3, 
            passport_number = $4, description = $5, file_path = $6
        WHERE portfolio_id = $7
    )";
    
    const char* params[7] = {
        std::to_string(portfolio.studentCode).c_str(),
        portfolio.date.c_str(),
        portfolio.passportSeries.c_str(),
        portfolio.passportNumber.c_str(),
        portfolio.description.c_str(),
        portfolio.filePath.c_str(),
        std::to_string(portfolio.portfolioId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 7, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка обновления портфолио: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

bool DatabaseService::deletePortfolio(int portfolioId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "DELETE FROM student_portfolio WHERE portfolio_id = $1";
    const char* params[1] = { std::to_string(portfolioId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка удаления портфолио: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

StudentPortfolio DatabaseService::getPortfolioById(int portfolioId) {
    StudentPortfolio portfolio;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return portfolio;
    }
    
    std::string sql = R"(
        SELECT portfolio_id, student_code, measure_code, date, passport_series, 
               passport_number, description, file_path
        FROM student_portfolio 
        WHERE portfolio_id = $1
    )";
    
    const char* params[1] = { std::to_string(portfolioId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return portfolio;
    }
    
    portfolio.portfolioId = std::stoi(PQgetvalue(res, 0, 0));
    portfolio.studentCode = std::stoi(PQgetvalue(res, 0, 1));
    portfolio.measureCode = std::stoi(PQgetvalue(res, 0, 2)); // ИСПРАВЛЕНО: добавлен std::stoi
    portfolio.date = PQgetvalue(res, 0, 3);
    portfolio.passportSeries = PQgetvalue(res, 0, 4);
    portfolio.passportNumber = PQgetvalue(res, 0, 5);
    portfolio.description = PQgetvalue(res, 0, 6);
    portfolio.filePath = PQgetvalue(res, 0, 7);
    
    PQclear(res);
    return portfolio;
}

// Event management - полный CRUD
std::vector<Event> DatabaseService::getEvents() {
    std::vector<Event> events;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return events;
    }
    
    // Обновленный запрос с JOIN для категорий и measure_code
    std::string sql = R"(
        SELECT e.event_id, e.measure_code, e.event_category_id, 
               ec.name as category_name, e.event_type, e.start_date, 
               e.end_date, e.location, e.lore, e.max_participants, 
               e.current_participants, e.status
        FROM event e
        LEFT JOIN event_category ec ON e.event_category_id = ec.event_category_id
        ORDER BY e.start_date DESC
    )";
    
    PGresult* res = PQexec(connection, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка выполнения запроса событий: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return events;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Event event;
        event.eventId = std::stoi(PQgetvalue(res, i, 0));
        event.measureCode = std::stoi(PQgetvalue(res, i, 1)); // связь с портфолио
        event.eventCategoryId = std::stoi(PQgetvalue(res, i, 2));
        event.categoryName = PQgetvalue(res, i, 3);
        event.eventType = PQgetvalue(res, i, 4);
        event.startDate = PQgetvalue(res, i, 5);
        event.endDate = PQgetvalue(res, i, 6);
        event.location = PQgetvalue(res, i, 7);
        event.lore = PQgetvalue(res, i, 8);
        event.maxParticipants = std::stoi(PQgetvalue(res, i, 9));
        event.currentParticipants = std::stoi(PQgetvalue(res, i, 10));
        event.status = PQgetvalue(res, i, 11);
        
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
    
    std::string sql = R"(
        INSERT INTO event 
        (measure_code, event_category_id, event_type, start_date, end_date, 
         location, lore, max_participants, current_participants, status) 
        VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)
    )";
    
    const char* params[10] = {
        std::to_string(event.measureCode).c_str(),
        std::to_string(event.eventCategoryId).c_str(),
        event.eventType.c_str(),
        event.startDate.c_str(),
        event.endDate.c_str(),
        event.location.c_str(),
        event.lore.c_str(),
        std::to_string(event.maxParticipants).c_str(),
        std::to_string(event.currentParticipants).c_str(),
        event.status.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 10, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка добавления события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

bool DatabaseService::updateEvent(const Event& event) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = R"(
        UPDATE event 
        SET measure_code = $1, event_category_id = $2, event_type = $3, 
            start_date = $4, end_date = $5, location = $6, lore = $7, 
            max_participants = $8, current_participants = $9, status = $10
        WHERE event_id = $11
    )";
    
    const char* params[11] = {
        std::to_string(event.measureCode).c_str(),
        std::to_string(event.eventCategoryId).c_str(),
        event.eventType.c_str(),
        event.startDate.c_str(),
        event.endDate.c_str(),
        event.location.c_str(),
        event.lore.c_str(),
        std::to_string(event.maxParticipants).c_str(),
        std::to_string(event.currentParticipants).c_str(),
        event.status.c_str(),
        std::to_string(event.eventId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 11, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка обновления события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

bool DatabaseService::deleteEvent(int eventId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "DELETE FROM event WHERE event_id = $1";
    const char* params[1] = { std::to_string(eventId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка удаления события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

Event DatabaseService::getEventById(int eventId) {
    Event event;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return event;
    }
    
    std::string sql = R"(
        SELECT event_id, measure_code, event_category_id, event_type, 
               start_date, end_date, location, lore, max_participants, 
               current_participants, status
        FROM event 
        WHERE event_id = $1
    )";
    
    const char* params[1] = { std::to_string(eventId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return event;
    }
    
    event.eventId = std::stoi(PQgetvalue(res, 0, 0));
    event.measureCode = std::stoi(PQgetvalue(res, 0, 1));
    event.eventCategoryId = std::stoi(PQgetvalue(res, 0, 2));
    event.eventType = PQgetvalue(res, 0, 3);
    event.startDate = PQgetvalue(res, 0, 4);
    event.endDate = PQgetvalue(res, 0, 5);
    event.location = PQgetvalue(res, 0, 6);
    event.lore = PQgetvalue(res, 0, 7);
    event.maxParticipants = std::stoi(PQgetvalue(res, 0, 8));
    event.currentParticipants = std::stoi(PQgetvalue(res, 0, 9));
    event.status = PQgetvalue(res, 0, 10);
    
    PQclear(res);
    return event;
}

// Event Category management - полный CRUD
std::vector<EventCategory> DatabaseService::getEventCategories() {
    std::vector<EventCategory> categories;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return categories;
    }
    
    PGresult* res = PQexec(connection, "SELECT event_category_id, name, description FROM event_category ORDER BY name");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка выполнения запроса категорий событий: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return categories;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        EventCategory category;
        category.eventCategoryId = std::stoi(PQgetvalue(res, i, 0));
        category.name = PQgetvalue(res, i, 1);
        category.description = PQgetvalue(res, i, 2);
        categories.push_back(category);
    }
    
    PQclear(res);
    return categories;
}

bool DatabaseService::addEventCategory(const EventCategory& category) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO event_category (name, description) VALUES ($1, $2)";
    const char* params[2] = {
        category.name.c_str(),
        category.description.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 2, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка добавления категории события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

bool DatabaseService::updateEventCategory(const EventCategory& category) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "UPDATE event_category SET name = $1, description = $2 WHERE event_category_id = $3";
    const char* params[3] = {
        category.name.c_str(),
        category.description.c_str(),
        std::to_string(category.eventCategoryId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 3, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка обновления категории события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

bool DatabaseService::deleteEventCategory(int eventCategoryId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "DELETE FROM event_category WHERE event_category_id = $1";
    const char* params[1] = { std::to_string(eventCategoryId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка удаления категории события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

EventCategory DatabaseService::getEventCategoryById(int eventCategoryId) {
    EventCategory category;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return category;
    }
    
    std::string sql = "SELECT event_category_id, name, description FROM event_category WHERE event_category_id = $1";
    const char* params[1] = { std::to_string(eventCategoryId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return category;
    }
    
    category.eventCategoryId = std::stoi(PQgetvalue(res, 0, 0));
    category.name = PQgetvalue(res, 0, 1);
    category.description = PQgetvalue(res, 0, 2);
    
    PQclear(res);
    return category;
}