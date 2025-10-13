#ifndef MODELS_H
#define MODELS_H

#include <string>

struct DatabaseConfig {
    std::string language = "en";
    std::string host = "localhost";
    int port = 5432;
    std::string database = "student_db";
    std::string username = "postgres";
    std::string password = "password";
};

struct ApiConfig {
    int port = 5000;
    std::string host = "0.0.0.0";
    int maxConnections = 10;
    int sessionTimeoutHours = 24;
    int resetTokenTimeoutMinutes = 60;
    bool enableCors = true;
    std::string corsOrigin = "*";
    bool enableSSL = false;
    std::string sslCertPath = "";
    std::string sslKeyPath = "";
    int rateLimitRequests = 100;
    int rateLimitWindow = 60; // seconds
};

// User structure
struct User {
    int userId = 0;
    std::string email;
    std::string phoneNumber;
    std::string passwordHash;
    std::string lastName;
    std::string firstName;
    std::string middleName;
};

// Teacher structure
struct Teacher {
    int teacherId = 0;
    std::string lastName;
    std::string firstName;
    std::string middleName;
    int experience = 0;
    std::string specialization;
    std::string email;
    std::string phoneNumber;
};

// Student structure
struct Student {
    int studentCode = 0;
    std::string lastName;
    std::string firstName;
    std::string middleName;
    std::string phoneNumber;
    std::string email;
    int groupId = 0;
};

// StudentGroup structure
struct StudentGroup {
    int groupId = 0;
    std::string name;
    int studentCount = 0;
    int teacherId = 0;
};

// StudentPortfolio structure
struct StudentPortfolio {
    int portfolioId = 0;
    int studentCode = 0;
    std::string measureCode;
    std::string date;
    std::string passportSeries;
    std::string passportNumber;
};

// Event structure
struct Event {
    int eventId = 0;
    std::string eventCategory;
    std::string eventType;
    std::string startDate;
    std::string endDate;
    std::string location;
    std::string lore;
};

#endif