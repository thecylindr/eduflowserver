#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <chrono>
#include <vector>

struct DatabaseConfig {
    std::string language;
    std::string host;
    int port;
    std::string database;
    std::string username;
    std::string password;
};

struct ApiConfig {
    int port;
    std::string host;
    int maxConnections;
    int sessionTimeoutHours;
    int resetTokenTimeoutMinutes;
    bool enableCors;
    std::string corsOrigin;
    bool enableSSL;
    std::string sslCertPath;
    std::string sslKeyPath;
    int rateLimitRequests;
    int rateLimitWindow;
};

struct User {
    int userId;
    std::string login;
    std::string email;
    std::string passwordHash;
    std::string firstName;
    std::string lastName;
    std::string middleName;
    std::string phoneNumber;
    std::string createdAt;
    std::string updatedAt;
};

struct Specialization {
    int specializationCode;
    std::string name;
};

struct Teacher {
    int teacherId;
    std::string lastName;
    std::string firstName;
    std::string middleName;
    int experience;
    std::string email;
    std::string phoneNumber;
    std::string specialization;  // Строка для отображения
    std::vector<Specialization> specializations;  // Для хранения списка специализаций
    int specializationCode;  // Добавьте эту строку - код специализации
};

struct Student {
    int studentCode;
    std::string lastName;
    std::string firstName;
    std::string middleName;
    std::string phoneNumber;
    std::string email;
    int groupId;
    std::string passportSeries;
    std::string passportNumber;
};

struct StudentGroup {
    int groupId;
    std::string name;
    int studentCount;
    int teacherId;
};

struct StudentPortfolio {
    int portfolioId;
    int studentCode;
    std::string measureCode;
    std::string date;
    std::string passportSeries;
    std::string passportNumber;
};

struct Event {
    int eventId;
    std::string eventCategory;
    std::string eventType;
    std::string startDate;
    std::string endDate;
    std::string location;
    std::string lore;
};

struct Session {
    std::string userId;
    std::string email;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastActivity;
};

struct PasswordResetToken {
    std::string email;
    std::chrono::system_clock::time_point createdAt;
};

#endif