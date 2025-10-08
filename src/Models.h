#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>

struct DatabaseConfig {
    std::string language = "en";
    std::string host = "localhost";
    int port = 5432;
    std::string database = "student_db";
    std::string username = "postgres";
    std::string password = "password";
    
    std::string getConnectionString() const {
        return "host=" + host + " port=" + std::to_string(port) + 
               " dbname=" + database + " user=" + username + 
               " password=" + password;
    }
};

struct Student {
    int studentCode;
    std::string lastName;
    std::string firstName;
    std::string middleName;
    std::string phoneNumber;
    std::string email;
    int groupId;
};

struct StudentGroup {
    int groupId;
    std::string name;
    int studentCount;
    int teacherId;
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

struct StudentPortfolio {
    int portfolioId;
    int studentCode;
    std::string measureCode;
    std::string date;
    std::string passportSeries;
    std::string passportNumber;
};

struct User {
    int userId;
    std::string email;
    std::string phoneNumber;
    std::string passwordHash;
    std::string lastName;
    std::string firstName;
    std::string middleName;
  
    User() : userId(0) {}
};

#endif