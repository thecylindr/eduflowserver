#include "database/DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>
#include "logger/logger.h"

// Group management
bool DatabaseService::updateGroupStudentCount(int groupId, int change) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // ИСПРАВЛЕНО: заменили "groups" на "student_groups"
    std::string sql = "UPDATE student_groups SET student_count = student_count + $1 WHERE group_id = $2";
    const char* params[2] = {
        std::to_string(change).c_str(),
        std::to_string(groupId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 2, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    PQclear(res);
    return success;
}

bool DatabaseService::recalculateAllGroupCounts() {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // ИСПРАВЛЕНО: заменили "groups" на "student_groups"
    std::string resetSql = "UPDATE student_groups SET student_count = 0";
    PGresult* resetRes = PQexec(connection, resetSql.c_str());
    bool resetSuccess = (PQresultStatus(resetRes) == PGRES_COMMAND_OK);
    PQclear(resetRes);
    
    if (!resetSuccess) {
        Logger::getInstance().log("❌ Ошибка сброса счетчиков групп: " + std::string(PQerrorMessage(connection)), "ERROR");
        return false;
    }
    
    // ИСПРАВЛЕНО: заменили "groups" на "student_groups"
    std::string updateSql = 
        "UPDATE student_groups g SET student_count = ("
        "    SELECT COUNT(*) FROM students s WHERE s.group_id = g.group_id"
        ")";
    
    PGresult* updateRes = PQexec(connection, updateSql.c_str());
    bool updateSuccess = (PQresultStatus(updateRes) == PGRES_COMMAND_OK);
    
    PQclear(updateRes);
    return updateSuccess;
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