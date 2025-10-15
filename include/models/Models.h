#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <chrono>

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
    std::string email;
    std::string login;
    std::string phoneNumber;
    std::string passwordHash;
    std::string lastName;
    std::string firstName;
    std::string middleName;
};

struct Teacher {
    int teacherId;
    std::string lastName;
    std::string firstName;
    std::string middleName;
    int experience;
    std::string specialization;
    std::string email;
    std::string phoneNumber;
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
};

struct PasswordResetToken {
    std::string email;
    std::chrono::system_clock::time_point createdAt;
};

#endif