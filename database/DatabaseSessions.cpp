#include "database/DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
std::chrono::system_clock::time_point stringToTimestamp(const std::string& str) {
std::tm tm = {};
std::stringstream ss(str);
ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
std::time_t t = std::mktime(&tm);
return std::chrono::system_clock::from_time_t(t);
}
std::string timestampToString(const std::chrono::system_clock::time_point& tp) {
std::time_t t = std::chrono::system_clock::to_time_t(tp);
std::stringstream ss;
ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
return ss.str();
}
bool DatabaseService::addSession(const Session& session) {
configManager.loadConfig(currentConfig);
if (!connection && !connect(currentConfig)) {
return false;
}
std::string sql = "INSERT INTO sessions (token, user_id, created_at, last_activity, expires_at, ip_address, user_agent) "
"VALUES ($1, $2, $3, $4, $5, $6, $7)";
std::string created = timestampToString(session.createdAt);
std::string last = timestampToString(session.lastActivity);
std::string expires = timestampToString(session.expiresAt);
const char* params[7] = {
session.token.c_str(),
session.userId.c_str(),
created.c_str(),
last.c_str(),
expires.c_str(),
session.ipAddress.c_str(),
session.userOS.c_str()
};
PGresult* res = PQexecParams(connection, sql.c_str(), 7, NULL, params, NULL, NULL, 0);
if (PQresultStatus(res) != PGRES_COMMAND_OK) {
std::cerr << "SQL error: " << PQerrorMessage(connection) << std::endl;
PQclear(res);
return false;
}
PQclear(res);
return true;
}
Session DatabaseService::getSessionByToken(const std::string& token) {
    Session session;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return session;
    }

    std::string sql = "SELECT session_id, token, user_id, created_at, last_activity, expires_at, ip_address, user_agent "
    "FROM sessions WHERE token = $1";
    const char* params[1] = { token.c_str() };
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return session;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return session;
    }
    
    session.sessionId = std::stoi(PQgetvalue(res, 0, 0));
    session.token = PQgetvalue(res, 0, 1);
    session.userId = PQgetvalue(res, 0, 2);
    session.createdAt = stringToTimestamp(PQgetvalue(res, 0, 3));
    session.lastActivity = stringToTimestamp(PQgetvalue(res, 0, 4));
    session.expiresAt = stringToTimestamp(PQgetvalue(res, 0, 5));
    session.ipAddress = PQgetvalue(res, 0, 6);
    session.userOS = PQgetvalue(res, 0, 7);
    
    PQclear(res);
    return session;
}

bool DatabaseService::updateSessionLastActivity(const std::string& token, const std::chrono::system_clock::time_point& newLastActivity, const std::chrono::system_clock::time_point& newExpiresAt) {
    configManager.loadConfig(currentConfig);
    if (!connection && !connect(currentConfig)) {
    return false;
    }
    std::string sql = "UPDATE sessions SET last_activity = $1, expires_at = $2 WHERE token = $3";
    std::string last = timestampToString(newLastActivity);
    std::string expires = timestampToString(newExpiresAt);
    const char* params[3] = {
    last.c_str(),
    expires.c_str(),
    token.c_str()
    };
    PGresult* res = PQexecParams(connection, sql.c_str(), 3, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return success;
    }
    bool DatabaseService::deleteSession(const std::string& token) {
    configManager.loadConfig(currentConfig);
    if (!connection && !connect(currentConfig)) {
    return false;
    }
    std::string sql = "DELETE FROM sessions WHERE token = $1";
    const char* params[1] = { token.c_str() };
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return success;
}

std::vector<Session> DatabaseService::getSessionsByUserId(const std::string& userId) {
std::vector<Session> sessionsList;
configManager.loadConfig(currentConfig);
if (!connection && !connect(currentConfig)) {
return sessionsList;
}
std::string sql = "SELECT session_id, token, user_id, created_at, last_activity, expires_at, ip_address, user_agent "
"FROM sessions WHERE user_id = $1";
const char* params[1] = { userId.c_str() };
PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
if (PQresultStatus(res) != PGRES_TUPLES_OK) {
PQclear(res);
return sessionsList;
}
int rows = PQntuples(res);
for (int i = 0; i < rows; i++) {
Session session;
session.sessionId = std::stoi(PQgetvalue(res, i, 0));
session.token = PQgetvalue(res, i, 1);
session.userId = PQgetvalue(res, i, 2);
session.createdAt = stringToTimestamp(PQgetvalue(res, i, 3));
session.lastActivity = stringToTimestamp(PQgetvalue(res, i, 4));
session.expiresAt = stringToTimestamp(PQgetvalue(res, i, 5));
session.ipAddress = PQgetvalue(res, i, 6);
session.userOS = PQgetvalue(res, i, 7);
sessionsList.push_back(session);
}
PQclear(res);
return sessionsList;
}
std::vector<Session> DatabaseService::getAllActiveSessions() {
std::vector<Session> sessionsList;
configManager.loadConfig(currentConfig);
if (!connection && !connect(currentConfig)) {
return sessionsList;
}
std::string sql = "SELECT session_id, token, user_id, created_at, last_activity, expires_at, ip_address, user_agent "
"FROM sessions WHERE expires_at > CURRENT_TIMESTAMP";
PGresult* res = PQexec(connection, sql.c_str());
if (PQresultStatus(res) != PGRES_TUPLES_OK) {
PQclear(res);
return sessionsList;
}
int rows = PQntuples(res);
for (int i = 0; i < rows; i++) {
Session session;
session.sessionId = std::stoi(PQgetvalue(res, i, 0));
session.token = PQgetvalue(res, i, 1);
session.userId = PQgetvalue(res, i, 2);
session.createdAt = stringToTimestamp(PQgetvalue(res, i, 3));
session.lastActivity = stringToTimestamp(PQgetvalue(res, i, 4));
session.expiresAt = stringToTimestamp(PQgetvalue(res, i, 5));
session.ipAddress = PQgetvalue(res, i, 6);
session.userOS = PQgetvalue(res, i, 7);
sessionsList.push_back(session);
}
PQclear(res);
return sessionsList;
}
bool DatabaseService::deleteExpiredSessions() {
configManager.loadConfig(currentConfig);
if (!connection && !connect(currentConfig)) {
return false;
}
std::string sql = "DELETE FROM sessions WHERE expires_at < CURRENT_TIMESTAMP";
PGresult* res = PQexec(connection, sql.c_str());
bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
PQclear(res);
return success;
}