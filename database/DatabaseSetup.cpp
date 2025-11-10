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
        "measure_code SERIAL NOT NULL UNIQUE,"
        "date DATE NOT NULL,"
        "decree INTEGER NOT NULL)",
        
        "CREATE TABLE IF NOT EXISTS event ("
        "id SERIAL PRIMARY KEY,"
        "event_id INTEGER REFERENCES student_portfolio(measure_code),"
        "event_category VARCHAR(24) NOT NULL UNIQUE,"
        "event_type VARCHAR(48) NOT NULL,"
        "start_date TIMESTAMP WITHOUT TIME ZONE NOT NULL,"
        "end_date TIMESTAMP WITHOUT TIME ZONE NOT NULL,"
        "location VARCHAR(24),"
        "lore TEXT)",
        
        "CREATE TABLE IF NOT EXISTS event_category ("
        "event_type VARCHAR(24) REFERENCES event(event_category),"
        "category VARCHAR(64))",
        
        "CREATE TABLE IF NOT EXISTS users ("
        "user_id SERIAL PRIMARY KEY,"
        "email VARCHAR(100) UNIQUE NOT NULL,"
        "login VARCHAR(24) NOT NULL,"
        "phone_number VARCHAR(20),"
        "password_hash TEXT NOT NULL,"
        "last_name VARCHAR(100) NOT NULL,"
        "first_name VARCHAR(100) NOT NULL,"
        "middle_name VARCHAR(100))",

        "CREATE TABLE IF NOT EXISTS sessions ("
        "session_id SERIAL PRIMARY KEY,"
        "token VARCHAR(64) UNIQUE NOT NULL,"
        "user_id INTEGER REFERENCES users(user_id) ON DELETE CASCADE,"
        "created_at TIMESTAMP NOT NULL,"
        "last_activity TIMESTAMP NOT NULL,"
        "ip_address VARCHAR(45),"
        "user_agent TEXT,"
        "expires_at TIMESTAMP NOT NULL);"
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