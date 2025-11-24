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