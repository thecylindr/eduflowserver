#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include "configs/ConfigManager.h"
#include "models/Models.h"
#include <vector>
#include <string>
#include <libpq-fe.h>  // Добавьте этот заголовок

class DatabaseService {
public:
    DatabaseService();
    ~DatabaseService();

    bool connect(const DatabaseConfig& config);
    void disconnect();
    bool testConnection();
    bool setupDatabase();
    
    // User management
    bool addUser(const User& user);
    bool updateUser(const User& user);
    User getUserByEmail(const std::string& email);
    User getUserById(int userId);
    
    // Teacher management
    std::vector<Teacher> getTeachers();
    bool addTeacher(const Teacher& teacher);
    bool updateTeacher(const Teacher& teacher);
    bool deleteTeacher(int teacherId);
    Teacher getTeacherById(int teacherId);
    
    // Student management
    std::vector<Student> getStudents();
    bool addStudent(const Student& student);
    bool updateStudent(const Student& student);
    bool deleteStudent(int studentId);
    Student getStudentById(int studentId);
    
    // Group management
    std::vector<StudentGroup> getGroups();
    bool addGroup(const StudentGroup& group);
    bool updateGroup(const StudentGroup& group);
    bool deleteGroup(int groupId);
    StudentGroup getGroupById(int groupId);
    
    // Portfolio management
    std::vector<StudentPortfolio> getPortfolios();
    bool addPortfolio(const StudentPortfolio& portfolio);
    
    // Event management
    std::vector<Event> getEvents();
    bool addEvent(const Event& event);
    
    // Specializations
    std::vector<std::string> getSpecializations();

    // Get current config
    DatabaseConfig getCurrentConfig() const { return currentConfig; }

private:
    void executeSQL(const std::string& sql);
    
    PGconn* connection;
    DatabaseConfig currentConfig;
    ConfigManager configManager;
};

#endif