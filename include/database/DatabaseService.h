#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include "configs/ConfigManager.h"
#include "models/Models.h"
#include <vector>
#include <libpq-fe.h>  // Добавляем заголовок PostgreSQL

class DatabaseService {
private:
    PGconn* connection;
    ConfigManager configManager;
    DatabaseConfig currentConfig;

public:
    DatabaseService();
    ~DatabaseService();

    bool connect(const DatabaseConfig& config);
    void disconnect();
    bool testConnection();
    bool setupDatabase();
    void executeSQL(const std::string& sql);

    // User management
    bool addUser(const User& user);
    bool updateUser(const User& user);
    User getUserByEmail(const std::string& email);
    User getUserById(int userId);

    // Teacher operations
    std::vector<Teacher> getTeachers();
    bool addTeacher(const Teacher& teacher);
    bool updateTeacher(const Teacher& teacher);
    bool deleteTeacher(int teacherId);
    Teacher getTeacherById(int teacherId);

    // Student operations
    std::vector<Student> getStudents();
    bool addStudent(const Student& student);
    bool updateStudent(const Student& student);
    bool deleteStudent(int studentCode);
    Student getStudentById(int studentCode);

    // Group operations
    std::vector<StudentGroup> getGroups();
    bool addGroup(const StudentGroup& group);
    bool updateGroup(const StudentGroup& group);
    bool deleteGroup(int groupId);
    StudentGroup getGroupById(int groupId);

    // Portfolio operations
    std::vector<StudentPortfolio> getPortfolios();
    bool addPortfolio(const StudentPortfolio& portfolio);

    // Event operations
    std::vector<Event> getEvents();
    bool addEvent(const Event& event);

    // Specialization operations
    std::vector<std::string> getSpecializations();

    // Добавляем публичный метод для получения конфигурации
    DatabaseConfig getCurrentConfig() const { return currentConfig; }
};

#endif