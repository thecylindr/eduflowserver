#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include "Models.h"
#include "ConfigManager.h"
#include <string>
#include <vector>

// Вместо прямого включения libpq-fe.h используем forward declaration
typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;

class DatabaseService {
public:
    DatabaseService();
    ~DatabaseService();
    
    bool testConnection();
    bool setupDatabase();
    bool connect(const DatabaseConfig& config);
    void disconnect();
    
    // CRUD операции
    bool addStudent(const Student& student);
    bool addTeacher(const Teacher& teacher);
    bool addGroup(const StudentGroup& group);
    bool addPortfolio(const StudentPortfolio& portfolio);
    
    std::vector<Student> getStudents();
    std::vector<Teacher> getTeachers();
    std::vector<StudentGroup> getGroups();
    std::vector<StudentPortfolio> getPortfolios();
    
    DatabaseConfig getCurrentConfig() const { return currentConfig; }

private:
    PGconn* connection;
    DatabaseConfig currentConfig;
    ConfigManager configManager;
    
    void executeSQL(const std::string& sql);
};

#endif