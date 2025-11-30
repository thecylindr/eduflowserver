#include "database/DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>
#include "logger/logger.h"

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
        Logger::getInstance().log("❌ Ошибка выполнения запроса getStudents: " + std::string(PQerrorMessage(connection)), "ERROR");
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

// Student management
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
        Logger::getInstance().log("❌ Ошибка добавления студента: " + std::string(PQerrorMessage(connection)), "ERROR");
    }
    PQclear(res);
    
    // 2. Обновляем счетчик студентов в группе
    if (success && student.groupId > 0) {
        std::string updateSql = "UPDATE student_groups SET student_count = student_count + 1 WHERE group_id = $1";
        const char* updateParams[1] = { std::to_string(student.groupId).c_str() };
        
        PGresult* updateRes = PQexecParams(connection, updateSql.c_str(), 1, NULL, updateParams, NULL, NULL, 0);
        if (PQresultStatus(updateRes) != PGRES_COMMAND_OK) {
            success = false;
            Logger::getInstance().log("❌ Ошибка обновления счетчика группы: " + std::string(PQerrorMessage(connection)), "ERROR");
        }
        PQclear(updateRes);
    }
    
    // Завершаем транзакцию
    if (success) {
        PGresult* commitRes = PQexec(connection, "COMMIT");
        PQclear(commitRes);
    } else {
        PGresult* rollbackRes = PQexec(connection, "ROLLBACK");
        PQclear(rollbackRes);
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
        Logger::getInstance().log("❌ Ошибка обновления студента: " + std::string(PQerrorMessage(connection)), "ERROR");
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
                Logger::getInstance().log("❌ Ошибка уменьшения счетчика старой группы: " + std::string(PQerrorMessage(connection)), "ERROR");
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
                Logger::getInstance().log("❌ Ошибка увеличения счетчика новой группы: " + std::string(PQerrorMessage(connection)), "ERROR");
            }
            PQclear(increaseRes);
        }
    }
    
    // Завершаем транзакцию
    if (success) {
        PGresult* commitRes = PQexec(connection, "COMMIT");
        PQclear(commitRes);
    } else {
        PGresult* rollbackRes = PQexec(connection, "ROLLBACK");
        PQclear(rollbackRes);
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
        Logger::getInstance().log("❌ Ошибка удаления студента: " + std::string(PQerrorMessage(connection)), "ERROR");
    }
    PQclear(res);
    
    // 2. Обновляем счетчик студентов в группе (если студент был в группе)
    if (success && student.groupId > 0) {
        std::string updateSql = "UPDATE student_groups SET student_count = student_count - 1 WHERE group_id = $1";
        const char* updateParams[1] = { std::to_string(student.groupId).c_str() };
        
        PGresult* updateRes = PQexecParams(connection, updateSql.c_str(), 1, NULL, updateParams, NULL, NULL, 0);
        if (PQresultStatus(updateRes) != PGRES_COMMAND_OK) {
            success = false;
            Logger::getInstance().log("❌ Ошибка обновления счетчика группы: " + std::string(PQerrorMessage(connection)), "ERROR");
        }
        PQclear(updateRes);
    }
    
    // Завершаем транзакцию
    if (success) {
        PGresult* commitRes = PQexec(connection, "COMMIT");
        PQclear(commitRes);
    } else {
        PGresult* rollbackRes = PQexec(connection, "ROLLBACK");
        PQclear(rollbackRes);
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
            Logger::getInstance().log("❌ Ошибка синхронизации счетчика для группы " + std::to_string(group.groupId) 
                    + ": " + std::string(PQerrorMessage(connection)), "ERROR");
        }
        PQclear(res);
    }
    
    // Завершаем транзакцию
    if (success) {
        PGresult* commitRes = PQexec(connection, "COMMIT");
        PQclear(commitRes);
    } else {
        PGresult* rollbackRes = PQexec(connection, "ROLLBACK");
        PQclear(rollbackRes);
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

std::vector<Student> DatabaseService::getStudentsByGroup(int groupId) {
    std::vector<Student> students;
    configManager.loadConfig(currentConfig);

    if (!connection && !connect(currentConfig)) {
        return students;
    }

    std::string sql = "SELECT student_code, last_name, first_name, middle_name, phone_number, email, group_id, passport_series, passport_number FROM students WHERE group_id = $1";
    const char* params[1] = { std::to_string(groupId).c_str() };

    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return students;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Student student;
        student.studentCode = std::stoi(PQgetvalue(res, i, 0));
        student.lastName = PQgetvalue(res, i, 1);
        student.firstName = PQgetvalue(res, i, 2);
        student.middleName = PQgetvalue(res, i, 3);
        student.phoneNumber = PQgetvalue(res, i, 4);
        student.email = PQgetvalue(res, i, 5);
        student.groupId = std::stoi(PQgetvalue(res, i, 6));
        student.passportSeries = PQgetvalue(res, i, 7);
        student.passportNumber = PQgetvalue(res, i, 8);

        students.push_back(student);
    }

    PQclear(res);
    return students;
}