#include "database/DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include "logger/logger.h"

// Statistics methods
int DatabaseService::getTeachersCount() {
    PGresult* res = PQexec(this->connection, "SELECT COUNT(*) FROM teachers;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::getInstance().log("❌ Database error in getTeachersCount: " + std::string(PQerrorMessage(connection)), "ERROR");
        PQclear(res);
        return 0;
    }
    int count = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count;
}

int DatabaseService::getStudentsCount() {
    PGresult* res = PQexec(this->connection, "SELECT COUNT(*) FROM students;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::getInstance().log("❌ Database error in getStudentsCount: " + std::string(PQerrorMessage(connection)), "ERROR");
        PQclear(res);
        return 0;
    }
    int count = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count;
}

int DatabaseService::getGroupsCount() {
    PGresult* res = PQexec(this->connection, "SELECT COUNT(*) FROM student_groups;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::getInstance().log("❌ Database error in getGroupsCount: " + std::string(PQerrorMessage(connection)), "ERROR");
        PQclear(res);
        return 0;
    }
    int count = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count;
}

int DatabaseService::getPortfoliosCount() {
    PGresult* res = PQexec(this->connection, "SELECT COUNT(*) FROM student_portfolio;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::getInstance().log("❌ Database error in getPortfoliosCount: " + std::string(PQerrorMessage(connection)), "ERROR");
        PQclear(res);
        return 0;
    }
    int count = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count;
}

int DatabaseService::getEventsCount() {
    PGresult* res = PQexec(this->connection, "SELECT COUNT(*) FROM event;");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        Logger::getInstance().log("❌ Database error in getEventsCount: " + std::string(PQerrorMessage(connection)), "ERROR");
        PQclear(res);
        return 0;
    }
    int count = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count;
}