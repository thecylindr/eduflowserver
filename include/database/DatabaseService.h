#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include "configs/ConfigManager.h"
#include "models/Models.h"
#include <vector>
#include <string>
#include <libpq-fe.h>

class DatabaseService {
public:
    DatabaseService();
    ~DatabaseService();

    // Database setup and management
    bool connect(const DatabaseConfig& config);
    void disconnect();
    bool testConnection();
    bool setupDatabase();

    // DashBoard info
    int getTeachersCount();
    int getStudentsCount();
    int getGroupsCount();
    int getPortfoliosCount();
    int getEventsCount();
    
    // User management
    bool addUser(const User& user);
    bool updateUser(const User& user);
    User getUserByEmail(const std::string& email);
    User getUserByUsername(const std::string& username);
    User getUserById(int userId);
    User getUserByLogin(const std::string& login);
    User getUserByPhoneNumber(const std::string& phoneNumber);
    
    // Teacher management
    std::vector<Teacher> getTeachers();
    bool addTeacher(const Teacher& teacher);
    bool updateTeacher(const Teacher& teacher);
    bool deleteTeacher(int teacherId);
    bool removeAllTeacherSpecializations(int teacherId);
    Teacher getTeacherById(int teacherId);
    std::vector<std::string> getUniqueSpecializationNames();
    
    // Student management
    std::vector<Student> getStudents();
    bool addStudent(const Student& student);
    bool updateStudent(const Student& student);
    bool deleteStudent(int studentId);
    Student getStudentById(int studentId);
    int getStudentCountInGroup(int groupId);
    bool syncStudentCounts();
    std::vector<Student> getStudentsByGroup(int groupId);
    
    // Group management
    std::vector<StudentGroup> getGroups();
    bool addGroup(const StudentGroup& group);
    bool updateGroup(const StudentGroup& group);
    bool deleteGroup(int groupId);
    StudentGroup getGroupById(int groupId);
    
    // Portfolio management
    std::vector<StudentPortfolio> getPortfolios();
    bool addPortfolio(const StudentPortfolio& portfolio);
    bool updatePortfolio(const StudentPortfolio& portfolio);
    bool deletePortfolio(int portfolioId);
    StudentPortfolio getPortfolioById(int portfolioId);
    
    // Event management
    std::vector<Event> getEvents();
    bool addEvent(const Event& event);
    bool updateEvent(const Event& event);
    bool deleteEvent(int eventId);
    Event getEventById(int eventId);
    
    // Specializations management
    std::vector<Specialization> getSpecializations();
    bool addSpecialization(const Specialization& specialization);
    int getSpecializationCodeByName(const std::string& name);
    bool deleteSpecialization(int specializationCode);
    
    // Teacher specializations management
    bool addTeacherSpecialization(int teacherId, int specializationCode);
    bool removeTeacherSpecialization(int teacherId, int specializationCode);
    std::vector<Specialization> getTeacherSpecializations(int teacherId);
    
    // Event Category management
    std::vector<EventCategory> getEventCategories();
    bool addEventCategory(const EventCategory& category);
    EventCategory getEventCategoryByCode(int eventCode);
    bool updateEventCategory(const EventCategory& category);
    bool deleteEventCategory(int eventCode);
    
    // Get current config
    DatabaseConfig getCurrentConfig() const { return currentConfig; }

    // Sessions management
    bool addSession(const Session& session);
    bool createSession(const Session& session) { return addSession(session); } // Алиас для обратной совместимости
    Session getSessionByToken(const std::string& token);
    bool updateSessionLastActivity(const std::string& token, 
                                  const std::chrono::system_clock::time_point& newLastActivity,
                                  const std::chrono::system_clock::time_point& newExpiresAt);
    bool deleteSession(const std::string& token);
    std::vector<Session> getSessionsByUserId(const std::string& userId);
    std::vector<Session> getAllActiveSessions();
    bool deleteExpiredSessions();

private:
    void executeSQL(const std::string& sql);
    
    PGconn* connection;
    DatabaseConfig currentConfig;
    ConfigManager configManager;
};

#endif