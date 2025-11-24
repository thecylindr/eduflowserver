#include "database/DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>

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
    
    int teacherId = std::stoi(PQgetvalue(res, 0, 0));  // Используем переменную
    int specializationCode = std::stoi(PQgetvalue(res, 0, 1));
    PQclear(res);
    
    std::cout << "✅ Teacher added with ID: " << teacherId << ", specialization code: " << specializationCode << std::endl;  // Используем teacherId здесь
    
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
    
    // ИСПРАВЛЕНИЕ: Получаем код специализации преподавателя правильным способом
    Teacher teacher = getTeacherById(teacherId);
    if (teacher.teacherId == 0) {
        return false;
    }
    
    std::cout << "🗑️ Удаление всех специализаций преподавателя ID: " << teacherId 
              << " (код специализации: " << teacher.specializationCode << ")" << std::endl;
    
    // ИСПРАВЛЕНИЕ: Используем specializationCode вместо specialization
    std::string sql = "DELETE FROM specialization_list WHERE specialization = $1";
    const char* params[1] = { std::to_string(teacher.specializationCode).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (success) {
        std::cout << "✅ Все специализации преподавателя удалены" << std::endl;
    } else {
        std::cerr << "❌ Ошибка удаления специализаций: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}