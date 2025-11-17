#include "database/DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>

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
    
    // Получаем код специализации преподавателя из таблицы teachers
    std::string teacherSql = "SELECT specialization FROM teachers WHERE teacher_id = $1";
    const char* teacherParams[1] = { std::to_string(teacherId).c_str() };
    
    PGresult* teacherRes = PQexecParams(connection, teacherSql.c_str(), 1, NULL, teacherParams, NULL, NULL, 0);
    if (PQresultStatus(teacherRes) != PGRES_TUPLES_OK || PQntuples(teacherRes) == 0) {
        PQclear(teacherRes);
        return specializations;
    }
    
    int specializationCode = std::stoi(PQgetvalue(teacherRes, 0, 0));
    PQclear(teacherRes);
    
    if (specializationCode == 0) {
        return specializations;
    }
    
    // Получаем информацию о специализации
    std::string specSql = "SELECT specialization, name FROM specialization_list WHERE specialization = $1";
    const char* specParams[1] = { std::to_string(specializationCode).c_str() };
    
    PGresult* specRes = PQexecParams(connection, specSql.c_str(), 1, NULL, specParams, NULL, NULL, 0);
    if (PQresultStatus(specRes) == PGRES_TUPLES_OK) {
        int rows = PQntuples(specRes);
        for (int i = 0; i < rows; i++) {
            Specialization spec;
            spec.specializationCode = std::stoi(PQgetvalue(specRes, i, 0));
            spec.name = PQgetvalue(specRes, i, 1);
            specializations.push_back(spec);
        }
    }
    PQclear(specRes);
    
    return specializations;
}

int DatabaseService::getSpecializationCodeByName(const std::string& name) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return 0;
    }
    
    std::string sql = "SELECT specialization FROM specialization_list WHERE name = $1";
    const char* params[1] = { name.c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return 0;
    }
    
    int code = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    
    return code;
}

std::vector<std::string> DatabaseService::getUniqueSpecializationNames() {
    std::vector<std::string> names;
    configManager.loadConfig(currentConfig);

    if (!connection && !connect(currentConfig)) {
        return names;
    }

    std::string sql = "SELECT DISTINCT name FROM specialization_list ORDER BY name ASC;";  // DISTINCT для уникальности, ORDER BY для сортировки

    PGresult* res = PQexec(connection, sql.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка получения уникальных специализаций: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return names;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        std::string name = PQgetvalue(res, i, 0);
        names.push_back(name);
    }

    PQclear(res);
    return names;
}