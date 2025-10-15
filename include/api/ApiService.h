#ifndef APISERVICE_H
#define APISERVICE_H

#include "database/DatabaseService.h"
#include "configs/ConfigManager.h"
#include "models/Models.h"
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <random>  // Добавьте этот заголовок

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define SOCKET_TYPE SOCKET
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #define SOCKET_TYPE int
    #define INVALID_SOCKET_VAL -1
    #define CLOSE_SOCKET close
#endif

class ApiService {
public:
    ApiService(DatabaseService& dbService);
    ~ApiService();

    // Server management
    bool start();
    void stop();

    // Authentication methods
    std::string handleRegister(const std::string& body);
    std::string handleLogin(const std::string& body);
    std::string handleForgotPassword(const std::string& body);
    std::string handleResetPassword(const std::string& body);
    std::string handleLogout(const std::string& sessionToken);
    
    // Teacher CRUD operations
    std::string handleAddTeacher(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateTeacher(const std::string& body, int teacherId, const std::string& sessionToken);
    std::string handleDeleteTeacher(int teacherId, const std::string& sessionToken);
    
    // Student CRUD operations
    std::string handleAddStudent(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateStudent(const std::string& body, int studentId, const std::string& sessionToken);
    std::string handleDeleteStudent(int studentId, const std::string& sessionToken);
    
    // Group CRUD operations
    std::string handleAddGroup(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateGroup(const std::string& body, int groupId, const std::string& sessionToken);
    std::string handleDeleteGroup(int groupId, const std::string& sessionToken);
    
    // Portfolio operations
    std::string handleAddPortfolio(const std::string& body, const std::string& sessionToken);
    
    // Event operations
    std::string handleAddEvent(const std::string& body, const std::string& sessionToken);
    
    // Profile operations
    std::string handleUpdateProfile(const std::string& body, const std::string& sessionToken);

    // Data retrieval methods
    std::string getTeachersJson(const std::string& sessionToken);
    std::string getStudentsJson(const std::string& sessionToken);
    std::string getGroupsJson(const std::string& sessionToken);
    std::string getPortfolioJson(const std::string& sessionToken);
    std::string getEventsJson(const std::string& sessionToken);
    std::string getSpecializationsJson(const std::string& sessionToken);
    std::string getProfile(const std::string& sessionToken);
    
    // System methods
    std::string handleStatus();

private:
    // Network methods
    void initializeNetwork();
    void cleanupNetwork();
    void runServer();
    void handleClient(SOCKET_TYPE clientSocket);
    
    // Utility methods
    std::string createJsonResponse(const std::string& content, int statusCode = 200);
    std::string generateSessionToken();  // Вернули объявление
    bool validateSession(const std::string& token);
    std::string getUserIdFromSession(const std::string& token);
    std::string hashPassword(const std::string& password);

    // Dependencies
    DatabaseService& dbService;
    ConfigManager configManager;
    ApiConfig apiConfig;
    
    // Session management
    std::unordered_map<std::string, Session> sessions;
    std::unordered_map<std::string, PasswordResetToken> passwordResetTokens;
    std::mutex sessionsMutex;
    std::mutex passwordResetMutex;
    
    // Server state
    bool running;
    std::thread serverThread;
    SOCKET_TYPE serverSocket;
    SOCKET_TYPE shutdownSocket;
};

#endif