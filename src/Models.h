#ifndef MODELS_H
#define MODELS_H

#include <string>

struct Student {
    int studentCode;
    std::string lastName;
    std::string firstName;
    std::string middleName;
    std::string phoneNumber;
    std::string email;
    int groupId;
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

struct DatabaseConfig {
    std::string host;
    int port;
    std::string database;
    std::string username;
    std::string password;
    std::string language;
};

struct User {
    int userId;
    std::string email;
    std::string phoneNumber;
    std::string passwordHash;
    std::string lastName;
    std::string firstName;
    std::string middleName;
};

#endif