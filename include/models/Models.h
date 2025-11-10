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
    std::string specialization;
    std::vector<Specialization> specializations;
    int specializationCode;
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

struct Session {
    int sessionId;
    std::string token;
    std::string userId;
    std::string email;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastActivity;
    std::chrono::system_clock::time_point expiresAt;
    std::string ipAddress;
    std::string userOS;
};

struct PasswordResetToken {
    std::string email;
    std::chrono::system_clock::time_point createdAt;
};

struct StudentPortfolio {
    int portfolioId;
    int studentCode;
    int measureCode;
    std::string date;
    int decree;
    std::string studentName; 
};

struct EventCategory {
    std::string eventType;   // event_type (VARCHAR(24) - первичный ключ)
    std::string category;    // category (VARCHAR(64))
};

struct Event {
    int eventId;           // id в таблице event (SERIAL PRIMARY KEY)
    int measureCode;       // event_id (REFERENCES student_portfolio(measure_code))
    std::string eventCategory; // event_category (VARCHAR(24))
    std::string eventType; // event_type (VARCHAR(48))
    std::string startDate;
    std::string endDate;
    std::string location;  // VARCHAR(24)
    std::string lore;      // TEXT
};

#endif